#include <simplicity/math/MathFunctions.h>
#include <simplicity/model/ModelFactory.h>

#include "TerrainStreamer.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		TerrainStreamer::TerrainStreamer(unique_ptr<TerrainSource> source, const Vector2ui& mapSize,
										 const Vector2ui& chunkSize, float unitLength,
										 const vector<unsigned int>& borderPatchLengths) :
				chunk(chunkSize, borderPatchLengths),
				currentPosition(0, 0),
				initialized(false),
				mapSamples(),
				meshNorthWest(0, 0),
				source(move(source)),
				trackedEntity(nullptr),
				unitLength(unitLength),
				worldPosition(0.0f, 0.0f, 0.0f)
		{
			mapSamples = mapSize;
			mapSamples.X() += 1;
			mapSamples.Y() += 1;
		}

		void TerrainStreamer::execute(Entity& /* entity */)
		{
			if (trackedEntity != nullptr)
			{
				worldPosition = getPosition3(trackedEntity->getTransform());
			}

			Vector2i newPosition = toUnitPosition(worldPosition);
			Vector2i movement = newPosition - currentPosition;
			currentPosition = newPosition;

			if (!initialized)
			{
				// Make sure the terrain is fully loaded.
				movement = Vector2i(0, -chunk.getSize().Y());

				initialized = true;
			}

			if (movement.X() == 0 && movement.Y() == 0)
			{
				// Not enough movement to require terrain streaming.
				return;
			}

			Vector2ui absoluteMovement(abs(movement.X()), abs(movement.Y()));

			streamNorthSouth(newPosition, movement, absoluteMovement);
			streamEastWest(newPosition, movement, absoluteMovement);
		}

		Vector2i TerrainStreamer::getChunkNorthWest() const
		{
			Vector2i chunkNorthWest = currentPosition;

			chunkNorthWest.X() -= chunk.getSize().X() / 2;
			chunkNorthWest.Y() -= chunk.getSize().Y() / 2;

			return chunkNorthWest;
		}

		float TerrainStreamer::getHeight(const Vector3& position) const
		{
			Vector2i relativePosition = toUnitPosition(position) - getChunkNorthWest();

			if (relativePosition.X() < 0 || relativePosition.X() >= chunk.getSize().X() ||
				relativePosition.Y() < 0 || relativePosition.Y() >= chunk.getSize().Y())
			{
				return 0.0f;
			}

			Vector2i meshPosition = meshMove(meshNorthWest, relativePosition);

			float xLocal = remainder(abs(position.X()), unitLength);
			float zLocal = remainder(abs(position.Z()), unitLength);
			bool inFirstTriangle = xLocal < zLocal;

			const MeshData& meshData = chunk.getMesh()->getData();

			unsigned int vertexIndex = (meshPosition.Y() * chunk.getSize().X() + meshPosition.X()) * 4;
			Vector3 pointA = meshData.vertexData[vertexIndex].position;
			Vector3 pointB;
			Vector3 pointC;
			if (inFirstTriangle)
			{
				pointB = meshData.vertexData[vertexIndex + 1].position;
				pointC = meshData.vertexData[vertexIndex + 2].position;
			}
			else
			{
				pointB = meshData.vertexData[vertexIndex + 2].position;
				pointC = meshData.vertexData[vertexIndex + 3].position;
			}

			chunk.getMesh()->releaseData();

			Vector3 edge0 = pointB - pointA;
			Vector3 edge1 = pointC - pointA;
			Vector3 normal = crossProduct(edge0, edge1);

			// Solve the equation for the plane (ax + by + cz + d = 0) to find y.
			float d = dotProduct(normal, pointA) * -1.0f;
			float y = ((normal.X() * position.X() + normal.Z() * position.Z() + d) / normal.Y()) * -1.0f;

			/*Logs::debug("simplicity::terrain", "Terrain height at (%f, %f): %f (xLocal: %f, zLocal: %f, triangle: %d)",
					  pointA.X(), pointA.Z(), y, xLocal, zLocal, inFirstTriangle);

			Logs::debug("simplicity::terrain",
					  "Terrain height at (%f, %f): %f (between %f and %f (%f)) (meshPosition: (%i, %i))", position.X(),
					  position.Z(), y, pointB.Y(), pointA.Y(), pointC.Y(), meshPosition.X(), meshPosition.Y());*/

			return y;
		}

		Vector2i TerrainStreamer::meshMove(const Vector2i& position, const Vector2i& movement) const
		{
			Vector2i newPosition = position;

			newPosition.X() += chunk.getSize().X() + movement.X();
			newPosition.X() %= chunk.getSize().X();

			newPosition.Y() += chunk.getSize().Y() + movement.Y();
			newPosition.Y() %= chunk.getSize().Y();

			return newPosition;
		}

		void TerrainStreamer::onAddEntity(Entity& entity)
		{
			entity.addUniqueComponent(move(chunk.createMesh()));
		}

		void TerrainStreamer::setCenter(const Vector3& center)
		{
			worldPosition = center;
		}

		void TerrainStreamer::setTrackedEntity(const Entity& trackedEntity)
		{
			this->trackedEntity = &trackedEntity;
		}

		void TerrainStreamer::streamEastWest(const Vector2i& position, const Vector2i& movement,
											 const Vector2ui& absoluteMovement)
		{
			if (movement.X() == 0)
			{
				return;
			}

			Vector2i sectionNorthWest = getChunkNorthWest();
			if (movement.X() > 0)
			{
				sectionNorthWest.X() += chunk.getSize().X() - movement.X();
			}
			Vector2i sectionSize(absoluteMovement.X(), chunk.getSize().Y());

			vector<float> heightMap = source->getSectionHeights(sectionNorthWest, sectionSize);
			vector<Vector3> normalMap = source->getSectionNormals(sectionNorthWest, sectionSize);

			Vector2i meshPosition = meshNorthWest;
			meshNorthWest.X() = meshMove(meshPosition, movement).X();
			if (movement.X() < 0)
			{
				meshPosition.X() = meshNorthWest.X();
			}

			writeSection(heightMap, normalMap, chunk.getSize().Y(), absoluteMovement.X(), sectionNorthWest, meshPosition);
		}

		void TerrainStreamer::streamNorthSouth(const Vector2i& position, const Vector2i& movement,
											   const Vector2ui& absoluteMovement)
		{
			if (movement.Y() == 0)
			{
				return;
			}

			Vector2i sectionNorthWest = getChunkNorthWest();
			sectionNorthWest.X() -= movement.X();
			if (movement.Y() > 0)
			{
				sectionNorthWest.Y() += chunk.getSize().Y() - movement.Y();
			}
			Vector2i sectionSize(chunk.getSize().X(), absoluteMovement.Y());

			vector<float> heightMap = source->getSectionHeights(sectionNorthWest, sectionSize);
			vector<Vector3> normalMap = source->getSectionNormals(sectionNorthWest, sectionSize);

			Vector2i meshPosition = meshNorthWest;
			meshNorthWest.Y() = meshMove(meshPosition, movement).Y();
			if (movement.Y() < 0)
			{
				meshPosition.Y() = meshNorthWest.Y();
			}

			writeSection(heightMap, normalMap, absoluteMovement.Y(), chunk.getSize().X(), sectionNorthWest, meshPosition);
		}

		Vector2i TerrainStreamer::toUnitPosition(const Vector3& worldPosition) const
		{
			return Vector2i(static_cast<int>(round(worldPosition.X() / unitLength)),
								  static_cast<int>(round(worldPosition.Z() / unitLength)));
		}

		void TerrainStreamer::writeSection(const vector<float>& heightMap, const vector<Vector3>& normalMap,
										   unsigned int rows, unsigned int columns,
										   const Vector2i& sectionNorthWest, const Vector2i& meshNorthWest)
		{
			MeshData& meshData = chunk.getMesh()->getData(false);

			unsigned int rowSamples = columns + 1;
			for (unsigned int row = 0; row < rows; row++)
			{
				for (unsigned int column = 0; column < columns; column++)
				{
					unsigned int meshRow = (meshNorthWest.Y() + row) % chunk.getSize().Y();
					unsigned int meshColumn = (meshNorthWest.X() + column) % chunk.getSize().X();
					unsigned int vertexIndex = (meshRow * chunk.getSize().X() + meshColumn) * 4;

					Vector2 northWest;
					northWest.X() = static_cast<float>(sectionNorthWest.X()) + static_cast<float>(column);
					northWest.Y() = static_cast<float>(sectionNorthWest.Y()) + static_cast<float>(row);
					northWest *= unitLength;

					vector<float> heights;
					heights.push_back(heightMap[row * rowSamples + column]);
					heights.push_back(heightMap[(row + 1) * rowSamples + column]);
					heights.push_back(heightMap[(row + 1) * rowSamples + column + 1]);
					heights.push_back(heightMap[row * rowSamples + column + 1]);

					vector<Vector3> normals;
					normals.push_back(normalMap[row * rowSamples + column]);
					normals.push_back(normalMap[(row + 1) * rowSamples + column]);
					normals.push_back(normalMap[(row + 1) * rowSamples + column + 1]);
					normals.push_back(normalMap[row * rowSamples + column + 1]);

					Vector4 color(0.0f, 0.0f, 0.0f, 1.0f);
					if (rows == chunk.getSize().Y() && columns == chunk.getSize().X())
					{
						color.G() = 1.0f;

						if (row == 0 || column == 0 ||
								row == chunk.getSize().Y() - 1 || column == chunk.getSize().X() - 1)
						{
							color.B() = 1.0f;
							color.R() = 1.0f;
						}
					}
					else if (columns == chunk.getSize().X())
					{
						color.B() = 1.0f;
					}
					else
					{
						color.R() = 1.0f;
					}

					writeSquareVertices(meshData, vertexIndex, northWest, heights, normals, color);
				}
			}

			chunk.getMesh()->releaseData();
		}

		void TerrainStreamer::writeSquareVertices(MeshData& meshData, unsigned int vertexIndex,
												  const Vector2& northWest, const vector<float>& heights,
												  const vector<Vector3>& normals, const Vector4& color)
		{
			Vertex& vertex0 = meshData.vertexData[vertexIndex];
			Vertex& vertex1 = meshData.vertexData[vertexIndex + 1];
			Vertex& vertex2 = meshData.vertexData[vertexIndex + 2];
			Vertex& vertex3 = meshData.vertexData[vertexIndex + 3];

			vertex0.color = color;
			vertex1.color = color;
			vertex2.color = color;
			vertex3.color = color;

			vertex0.normal = normals[0];
			vertex1.normal = normals[1];
			vertex2.normal = normals[2];
			vertex3.normal = normals[3];

			vertex0.position.X() = northWest.X();
			vertex0.position.Y() = heights[0];
			vertex0.position.Z() = northWest.Y();
			vertex1.position.X() = northWest.X();
			vertex1.position.Y() = heights[1];
			vertex1.position.Z() = northWest.Y() + unitLength;
			vertex2.position.X() = northWest.X() + unitLength;
			vertex2.position.Y() = heights[2];
			vertex2.position.Z() = northWest.Y() + unitLength;
			vertex3.position.X() = northWest.X() + unitLength;
			vertex3.position.Y() = heights[3];
			vertex3.position.Z() = northWest.Y();

			vertex0.texCoord = Vector2(0.0f, 0.0f);
			vertex1.texCoord = Vector2(0.0f, 1.0f);
			vertex2.texCoord = Vector2(1.0f, 1.0f);
			vertex3.texCoord = Vector2(1.0f, 0.0f);

			/*vertex0.color = Vector4(getRandomInt(0, 1), getRandomInt(0, 1), getRandomInt(0, 1), 1.0f);
			vertex1.color = Vector4(getRandomInt(0, 1), getRandomInt(0, 1), getRandomInt(0, 1), 1.0f);
			vertex2.color = Vector4(getRandomInt(0, 1), getRandomInt(0, 1), getRandomInt(0, 1), 1.0f);
			vertex3.color = Vector4(getRandomInt(0, 1), getRandomInt(0, 1), getRandomInt(0, 1), 1.0f);*/

			float maxY = max(vertex0.position.Y(),
							 max(vertex1.position.Y(), max(vertex2.position.Y(), vertex3.position.Y())));

			vertex0.color = Vector4(0.0f, 0.5f, 0.0f, 1.0f);
			vertex1.color = Vector4(0.0f, 0.5f, 0.0f, 1.0f);
			vertex2.color = Vector4(0.0f, 0.5f, 0.0f, 1.0f);
			vertex3.color = Vector4(0.0f, 0.5f, 0.0f, 1.0f);

			// Snow!
			if (maxY > 60.0f)
			{
				vertex0.color = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
				vertex1.color = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
				vertex2.color = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
				vertex3.color = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
			}

			// Beaches!
			if (maxY < 2.0f)
			{
				vertex0.color = Vector4(0.83f, 0.65f, 0.15f, 1.0f);
				vertex1.color = Vector4(0.83f, 0.65f, 0.15f, 1.0f);
				vertex2.color = Vector4(0.83f, 0.65f, 0.15f, 1.0f);
				vertex3.color = Vector4(0.83f, 0.65f, 0.15f, 1.0f);
			}
		}
	}
}
