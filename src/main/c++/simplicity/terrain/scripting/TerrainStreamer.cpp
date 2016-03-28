#include <simplicity/math/MathFunctions.h>
#include <simplicity/model/ModelFactory.h>
#include <simplicity/Simplicity.h>

#include "TerrainStreamer.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		TerrainStreamer::TerrainStreamer(unique_ptr<vector<LevelOfDetail>> levelsOfDetail, const Vector2ui& mapSize,
										 unsigned int chunkSize) :
				centerPosition(0, 0),
				chunks(),
				chunkSize(chunkSize),
				layerMap(),
				levelsOfDetail(move(levelsOfDetail)),
				mapSize(mapSize),
				northWestChunk(0, 0),
				radiusf(0.0f),
				radiusi(0),
				targetEntity(nullptr),
				targetPosition(0.0f, 0.0f, 0.0f)
		{
			// TODO chunkSize must be even! Enforce it!
			// TODO Also things probably need to be multiples of each-other...

			unsigned int layer = 0;
			for (unsigned int index = 0; index < this->levelsOfDetail->size(); index++)
			{
				unsigned int maxLayer = layer + this->levelsOfDetail->at(index).layerCount;
				while (layer < maxLayer)
				{
					layerMap[layer] = index;
					layer++;
				}
			}

			radiusi = layer - 1;
			radiusf = radiusi + 0.5f;
		}

		void TerrainStreamer::execute()
		{
			if (targetEntity != nullptr)
			{
				targetPosition = getPosition3(targetEntity->getTransform());
			}

			float worldMovementX = targetPosition.X() - centerPosition.X();
			float worldMovementZ = targetPosition.Z() - centerPosition.Y();
			int movementX = static_cast<int>(round(worldMovementX / chunkSize));
			int movementY = static_cast<int>(round(worldMovementZ / chunkSize));

			if (movementX == 0 && movementY == 0)
			{
				// Not enough movement to require terrain streaming.
				return;
			}

			centerPosition.X() += movementX * chunkSize;
			centerPosition.Y() += movementY * chunkSize;

			stream(Vector2i(movementX, movementY));
		}

		void TerrainStreamer::onAddEntity()
		{
			unsigned int layerCount = 0;
			for (unsigned int index = 0; index < levelsOfDetail->size(); index++)
			{
				layerCount += levelsOfDetail->at(index).layerCount;
			}

			unsigned int size = layerCount * 2 - 1;
			chunks.reserve(size);
			for (unsigned int x = 0; x < size; x++)
			{
				chunks.push_back(vector<TerrainChunk>());
				chunks.back().reserve(size);

				for (unsigned int y = 0; y < size; y++)
				{
					chunks.back().push_back(TerrainChunk(0, 0));
				}
			}

			stream(Vector2i(static_cast<unsigned int>(chunks.size()),
							static_cast<unsigned int>(chunks.size())));
		}

		void TerrainStreamer::setTarget(const Entity& target)
		{
			targetEntity = &target;
		}

		void TerrainStreamer::setTarget(const Vector3& target)
		{
			targetEntity = nullptr;
			targetPosition = target;
		}

		void TerrainStreamer::stream(const Vector2i& movement)
		{
			unsigned int size = static_cast<unsigned int>(chunks.size());

			for (unsigned int x = 0; x < size; x++)
			{
				for (unsigned int y = 0; y < size; y++)
				{
					unsigned int previousX = (x + northWestChunk.X()) % size;
					unsigned int previousY = (y + northWestChunk.Y()) % size;

					unsigned int previousXDistance = max(previousX, radiusi) - min(previousX, radiusi);
					unsigned int previousYDistance = max(previousY, radiusi) - min(previousY, radiusi);
					unsigned int previousLayer = max(previousXDistance, previousYDistance);
					unsigned int previousLevelOfDetail = layerMap.at(previousLayer);

					int targetX = previousX - movement.X();
					int targetY = previousY - movement.Y();

					bool wrap = false;
					if (targetX < 0 || targetX >= size ||
						targetY < 0 || targetY >= size)
					{
						wrap = true;
					}

					unsigned int wrappedTargetX = (targetX + size) % size;
					unsigned int wrappedTargetY = (targetY + size) % size;

					unsigned int targetXDistance = max(wrappedTargetX, radiusi) - min(wrappedTargetX, radiusi);
					unsigned int targetYDistance = max(wrappedTargetY, radiusi) - min(wrappedTargetY, radiusi);
					unsigned int targetLayer = max(targetXDistance, targetYDistance);
					unsigned int targetLevelOfDetail = layerMap.at(targetLayer);

					int worldX = static_cast<int>((wrappedTargetX - radiusf) * chunkSize) + centerPosition.X();
					int worldY = static_cast<int>((wrappedTargetY - radiusf) * chunkSize) + centerPosition.Y();
					Vector2i chunkNorthWest(worldX, worldY);

					unsigned int scale = levelsOfDetail->at(targetLevelOfDetail).sampleFrequency;
					unsigned int scaledChunkSize = chunkSize / scale;
					Vector2i scaledChunkNorthWest = chunkNorthWest / static_cast<int>(scale);
					Vector2ui scaledChunkArea(scaledChunkSize, scaledChunkSize);

					//if (wrap || targetLevelOfDetail != previousLevelOfDetail)
					{

						getEntity()->removeComponent(*chunks[x][y].getMesh());
						chunks[x][y] = TerrainChunk(scaledChunkSize, scale);
						getEntity()->addComponent(move(chunks[x][y].createMesh()));

						TerrainSource* source = levelsOfDetail->at(targetLevelOfDetail).source.get();
						vector<float> heightMap = source->getSectionHeights(scaledChunkNorthWest, scaledChunkArea);
						vector<Vector3> normalMap = source->getSectionNormals(scaledChunkNorthWest, scaledChunkArea);

						chunks[x][y].setVertices(chunkNorthWest, heightMap, normalMap);
					}
					/*else
					{
						chunks[x][y].patch(TerrainChunk::Edge::NORTH, 1);
						chunks[x][y].patch(TerrainChunk::Edge::EAST, 1);
						chunks[x][y].patch(TerrainChunk::Edge::SOUTH, 1);
						chunks[x][y].patch(TerrainChunk::Edge::WEST, 1);
					}*/

					if (targetLevelOfDetail < levelsOfDetail->size() - 1 &&
						layerMap.at(targetLayer + 1) != targetLevelOfDetail)
					{
						unsigned int nextScale = levelsOfDetail->at(targetLevelOfDetail + 1).sampleFrequency;
						unsigned int scaleRatio = nextScale / scale;

						if (targetXDistance == targetLayer)
						{
							if (wrappedTargetX <= radiusi)
							{
								chunks[x][y].patch(TerrainChunk::Edge::WEST, scaleRatio);
							}
							if (wrappedTargetX >= radiusi)
							{
								chunks[x][y].patch(TerrainChunk::Edge::EAST, scaleRatio);
							}
						}
						if (targetYDistance == targetLayer)
						{
							if (wrappedTargetY <= radiusi)
							{
								chunks[x][y].patch(TerrainChunk::Edge::NORTH, scaleRatio);
							}
							if (wrappedTargetY >= radiusi)
							{
								chunks[x][y].patch(TerrainChunk::Edge::SOUTH, scaleRatio);
							}
						}
					}
				}
			}

			northWestChunk.X() = (northWestChunk.X() - movement.X() + size) % size;
			northWestChunk.Y() = (northWestChunk.Y() - movement.Y() + size) % size;
		}
	}
}
