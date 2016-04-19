#include <simplicity/math/MathFunctions.h>
#include <simplicity/model/ModelFactory.h>
#include <simplicity/Simplicity.h>

#include "TerrainStreamer.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		TerrainStreamer::TerrainStreamer(unique_ptr<TerrainSource> source, const Vector2ui& mapSize,
										 unsigned int chunkSize, const vector<LevelOfDetail>& lods) :
			chunks(),
			chunkSize(chunkSize),
			layerMap(),
			lods(lods),
			northWestChunk(0, 0),
			northWestPosition(0.0f, 0.0f, 0.0f),
			mapNorthWest(-static_cast<int>(mapSize.X()) / 2, -static_cast<int>(mapSize.Y()) / 2),
			mapSouthEast(mapSize.X() / 2 - chunkSize, mapSize.Y() / 2 - chunkSize),
			radius(0),
			size(0),
			source(move(source)),
			targetEntity(nullptr),
			targetPosition(0.0f, 0.0f, 0.0f)
		{
			// TODO chunkSize must be even! Enforce it!
			// TODO Also things probably need to be multiples of each-other...

			if (this->lods.size() == 0)
			{
				LevelOfDetail levelOfDetail;
				levelOfDetail.layerCount = 1;
				levelOfDetail.sampleFrequency = 1;
				this->lods.push_back(levelOfDetail);
			}

			unsigned int layer = 0;
			for (unsigned int index = 0; index < lods.size(); index++)
			{
				unsigned int maxLayer = layer + lods[index].layerCount;
				while (layer < maxLayer)
				{
					layerMap[layer] = index;
					layer++;
				}
			}

			radius = layer - 1;
			size = layer * 2 - 1;
			northWestPosition.X() -= (radius + 0.5f) * chunkSize;
			northWestPosition.Z() -= (radius + 0.5f) * chunkSize;
		}

		void TerrainStreamer::execute()
		{
			if (targetEntity != nullptr)
			{
				targetPosition = getPosition3(targetEntity->getTransform());
			}

			Vector3 relativePosition = toRelativePosition(targetPosition, true);
			Vector2i relativeChunkPosition = toChunkPosition(relativePosition);

			if (relativeChunkPosition.X() == 0 && relativeChunkPosition.Y() == 0)
			{
				// Not enough movement to require terrain streaming.
				return;
			}

			northWestPosition += toWorldPosition(relativeChunkPosition);

			stream(relativeChunkPosition);
		}

		float TerrainStreamer::getHeight(const Vector3& position) const
		{
			Vector3 relativePosition = toRelativePosition(position);
			Vector2i chunkPosition = toChunkPosition(relativePosition);
			if (chunkPosition.X() < 0 || chunkPosition.X() >= size ||
				chunkPosition.Y() < 0 || chunkPosition.Y() >= size)
			{
				return 0.0f;
			}

			// Relative to chunk.
			relativePosition -= toWorldPosition(chunkPosition);

			unsigned int x = (chunkPosition.X() + northWestChunk.X()) % size;
			unsigned int y = (chunkPosition.Y() + northWestChunk.Y()) % size;

			return chunks[x][y].getHeight(relativePosition);
		}

		void TerrainStreamer::onAddEntity()
		{
			chunks.reserve(size);
			for (unsigned int x = 0; x < size; x++)
			{
				chunks.push_back(vector<TerrainChunk>(size, TerrainChunk(0, 0)));
			}

			stream(Vector2i(size, size));
		}

		void TerrainStreamer::setTarget(const Entity& target)
		{
			targetEntity = &target;
			targetPosition = targetEntity->getPosition();
		}

		void TerrainStreamer::setTarget(const Vector3& target)
		{
			targetEntity = nullptr;
			targetPosition = target;
		}

		void TerrainStreamer::stream(const Vector2i& movement)
		{
			for (unsigned int x = 0; x < size; x++)
			{
				for (unsigned int y = 0; y < size; y++)
				{
					unsigned int previousX = (x - northWestChunk.X() + size) % size;
					unsigned int previousY = (y - northWestChunk.Y() + size) % size;

					unsigned int previousXDistance = max(previousX, radius) - min(previousX, radius);
					unsigned int previousYDistance = max(previousY, radius) - min(previousY, radius);
					unsigned int previousLayer = max(previousXDistance, previousYDistance);
					unsigned int previousLodIndex = layerMap.at(previousLayer);

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

					unsigned int targetXDistance = max(wrappedTargetX, radius) - min(wrappedTargetX, radius);
					unsigned int targetYDistance = max(wrappedTargetY, radius) - min(wrappedTargetY, radius);
					unsigned int targetLayer = max(targetXDistance, targetYDistance);
					unsigned int targetLodIndex = layerMap.at(targetLayer);

					int worldX = static_cast<int>(wrappedTargetX * chunkSize + northWestPosition.X());
					int worldY = static_cast<int>(wrappedTargetY * chunkSize + northWestPosition.Z());
					Vector2i chunkNorthWest(worldX, worldY);

					if (chunkNorthWest.X() < mapNorthWest.X() ||
						chunkNorthWest.Y() < mapNorthWest.Y() ||
						chunkNorthWest.X() > mapSouthEast.X() ||
						chunkNorthWest.Y() > mapSouthEast.Y())
					{
						if (chunks[x][y].getModel() != nullptr)
						{
							chunks[x][y].getModel()->setVisible(false);
						}

						continue;
					}

					unsigned int scale = lods[targetLodIndex].sampleFrequency;
					unsigned int scaledChunkSize = chunkSize / scale;
					Vector2i scaledChunkNorthWest = chunkNorthWest / static_cast<int>(scale);
					Vector2ui scaledChunkArea(scaledChunkSize, scaledChunkSize);

					if (wrap || targetLodIndex != previousLodIndex)
					{
						getEntity()->removeComponent(*chunks[x][y].getModel());
						chunks[x][y] = TerrainChunk(scaledChunkSize, scale);
						getEntity()->addComponent(move(chunks[x][y].createModel()));

						vector<float> heightMap = source->getSectionHeights(scaledChunkNorthWest, scaledChunkArea,
																			targetLodIndex);
						vector<Vector3> normalMap = source->getSectionNormals(scaledChunkNorthWest, scaledChunkArea,
																			  targetLodIndex);

						chunks[x][y].setVertices(chunkNorthWest, heightMap, normalMap);
					}
					else
					{
						chunks[x][y].patch(TerrainChunk::Edge::NORTH, 1);
						chunks[x][y].patch(TerrainChunk::Edge::EAST, 1);
						chunks[x][y].patch(TerrainChunk::Edge::SOUTH, 1);
						chunks[x][y].patch(TerrainChunk::Edge::WEST, 1);
					}

					if (targetLodIndex < lods.size() - 1 &&
						layerMap.at(targetLayer + 1) != targetLodIndex)
					{
						unsigned int nextScale = lods[targetLodIndex + 1].sampleFrequency;
						unsigned int scaleRatio = nextScale / scale;

						if (targetXDistance == targetLayer)
						{
							if (wrappedTargetX <= radius)
							{
								chunks[x][y].patch(TerrainChunk::Edge::WEST, scaleRatio);
							}
							if (wrappedTargetX >= radius)
							{
								chunks[x][y].patch(TerrainChunk::Edge::EAST, scaleRatio);
							}
						}
						if (targetYDistance == targetLayer)
						{
							if (wrappedTargetY <= radius)
							{
								chunks[x][y].patch(TerrainChunk::Edge::NORTH, scaleRatio);
							}
							if (wrappedTargetY >= radius)
							{
								chunks[x][y].patch(TerrainChunk::Edge::SOUTH, scaleRatio);
							}
						}
					}
				}
			}

			northWestChunk.X() = (northWestChunk.X() + movement.X() + size) % size;
			northWestChunk.Y() = (northWestChunk.Y() + movement.Y() + size) % size;
		}

		Vector2i TerrainStreamer::toChunkPosition(const Vector3& position) const
		{
			return Vector2i(static_cast<int>(floor(position.X() / chunkSize)),
							static_cast<int>(floor(position.Z() / chunkSize)));
		}

		Vector3 TerrainStreamer::toRelativePosition(const Vector3& position, bool relativeToCenterChunk) const
		{
			Vector3 relativePosition = position - northWestPosition;

			if (relativeToCenterChunk)
			{
				relativePosition -= Vector3(radius * chunkSize, 0.0f, radius * chunkSize);
			}

			return relativePosition;
		}

		Vector3 TerrainStreamer::toWorldPosition(const Vector2i& position) const
		{
			return Vector3(position.X() * static_cast<int>(chunkSize),
						   0.0f,
						   position.Y() * static_cast<int>(chunkSize));
		}
	}
}
