#include "../ResourceTerrainSource.h"
#include "LODTerrainStreamer.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		LODTerrainStreamer::LODTerrainStreamer(const Resource& resource, const Vector2ui& mapSize,
											   const vector<Vector2ui>& chunkSizes,
											   const vector<unsigned int>& sampleFrequencies) :
				chunkSizes(chunkSizes),
				currentPosition(0, 0),
				initialized(false),
				largestUnitLength(sampleFrequencies.back()),
				streamers(),
				trackedEntity(),
				worldPosition(0.0f, 0.0f, 0.0f)
		{
			for (unsigned int sampleFrequency : sampleFrequencies)
			{
				if (mapSize.X() % sampleFrequency != 0)
				{
					Logs::error("simplicity::terrain",
								"The sample frequencies must be a multiple of the dimensions of the map.");
					return;
				}
			}

			unsigned int resourceOffset = 0;
			unsigned int stride = 4;

			for (unsigned int lodIndex = 0; lodIndex < chunkSizes.size(); lodIndex++)
			{
				Vector2ui chunkSize = chunkSizes[lodIndex];
				unsigned int sampleFrequency = sampleFrequencies[lodIndex];
				unsigned int borderPatchSize = 1;
				if (lodIndex < chunkSizes.size() - 1)
				{
					unsigned int nextSampleFrequency = sampleFrequencies[lodIndex + 1];
					borderPatchSize = nextSampleFrequency / sampleFrequency;
				}
				Vector2ui lodMapSize = mapSize / sampleFrequency;

				if (lodIndex == 0)
				{
					unique_ptr<TerrainSource> source(new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(source), lodMapSize, chunkSize, sampleFrequency,
												{ borderPatchSize, borderPatchSize,
												  borderPatchSize, borderPatchSize })));
				}
				else
				{
					unsigned int highestSampleFrequency = sampleFrequencies[0];
					unsigned int sampleFrequencyRatio = sampleFrequency / highestSampleFrequency;

					Vector2ui previousChunkSize = chunkSizes[lodIndex - 1];
					Vector2ui outerSize = (chunkSize - previousChunkSize) / 2u;

					previousChunkSize /= sampleFrequencyRatio;
					outerSize /= sampleFrequencyRatio;

					// North
					unique_ptr<TerrainSource> northSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(northSource), lodMapSize,
												Vector2ui(previousChunkSize.X(), outerSize.Y()), sampleFrequency,
												{ borderPatchSize, 1, 1, 1 })));

					// North East
					unique_ptr<TerrainSource> northEastSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(northEastSource), lodMapSize,
												Vector2ui(outerSize.X(), outerSize.Y()), sampleFrequency,
												{ borderPatchSize, borderPatchSize, 1, 1 })));

					// East
					unique_ptr<TerrainSource> eastSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(eastSource), lodMapSize,
												Vector2ui(outerSize.X(), previousChunkSize.Y()), sampleFrequency,
												{ 1, borderPatchSize, 1, 1 })));

					// South East
					unique_ptr<TerrainSource> southEastSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(southEastSource), lodMapSize,
												Vector2ui(outerSize.X(), outerSize.Y()), sampleFrequency,
												{ 1, borderPatchSize, borderPatchSize, 1 })));

					// South
					unique_ptr<TerrainSource> southSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(southSource), lodMapSize,
												Vector2ui(previousChunkSize.X(), outerSize.Y()),sampleFrequency,
												{ 1, 1, borderPatchSize, 1 })));

					// South West
					unique_ptr<TerrainSource> southWestSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(southWestSource), lodMapSize,
												Vector2ui(outerSize.X(), outerSize.Y()), sampleFrequency,
												{ 1, 1, borderPatchSize, borderPatchSize })));

					// West
					unique_ptr<TerrainSource> westSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(westSource), lodMapSize,
												Vector2ui(outerSize.X(), previousChunkSize.Y()), sampleFrequency,
												{ 1, 1, 1, borderPatchSize })));

					// North West
					unique_ptr<TerrainSource> northWestSource(
							new ResourceTerrainSource(lodMapSize, resource, resourceOffset));
					streamers.push_back(unique_ptr<TerrainStreamer>(
							new TerrainStreamer(move(northWestSource), lodMapSize,
												Vector2ui(outerSize.X(),outerSize.Y()), sampleFrequency,
												{ borderPatchSize, 1, 1, borderPatchSize })));
				}

				Vector2ui lodMapSamples = lodMapSize;
				lodMapSamples.X() += 1;
				lodMapSamples.Y() += 1;

				resourceOffset += lodMapSamples.X() * lodMapSamples.Y() * sizeof(float) * stride;
			}
		}

		void LODTerrainStreamer::execute(Entity& entity)
		{
			if (trackedEntity != nullptr)
			{
				worldPosition = getPosition3(trackedEntity->getTransform());
			}

			Vector2i newPosition = toLargestUnitPosition(worldPosition);
			Vector2i movement = newPosition - currentPosition;

			if (initialized)
			{
				if (movement.X() == 0 && movement.Y() == 0)
				{
					// Not enough movement to require terrain streaming.
					return;
				}
			}
			else
			{
				initialized = true;
			}

			currentPosition = newPosition;
			Vector3 roundedWorldPosition(newPosition.X() * largestUnitLength, worldPosition.Y(), newPosition.Y() * largestUnitLength);

			unsigned int streamerIndex = 0;
			for (unsigned int lodIndex = 0; lodIndex < chunkSizes.size(); lodIndex++)
			{
				if (lodIndex == 0)
				{
					streamers[streamerIndex]->setCenter(roundedWorldPosition);
					streamers[streamerIndex++]->execute(entity);
				}
				else
				{
					Vector2ui halfChunkSize = chunkSizes[lodIndex] / 2u;
					Vector2ui halfPreviousChunkSize = chunkSizes[lodIndex - 1] / 2u;
					Vector2ui offset = halfPreviousChunkSize + (halfChunkSize - halfPreviousChunkSize) / 2u;

					// North
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(0.0f, 0.0f, -static_cast<int>(offset.Y())));
					streamers[streamerIndex++]->execute(entity);

					// North East
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(offset.X(), 0.0f, -static_cast<int>(offset.Y())));
					streamers[streamerIndex++]->execute(entity);

					// East
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(offset.X(), 0.0f, 0.0f));
					streamers[streamerIndex++]->execute(entity);

					// South East
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(offset.X(), 0.0f, offset.Y()));
					streamers[streamerIndex++]->execute(entity);

					// South
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(0.0f, 0.0f, offset.Y()));
					streamers[streamerIndex++]->execute(entity);

					// South West
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(-static_cast<int>(offset.X()), 0.0f, offset.Y()));
					streamers[streamerIndex++]->execute(entity);

					// West
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(-static_cast<int>(offset.X()), 0.0f, 0.0f));
					streamers[streamerIndex++]->execute(entity);

					// North West
					streamers[streamerIndex]->setCenter(
							roundedWorldPosition + Vector3(-static_cast<int>(offset.X()), 0.0f, -static_cast<int>(offset.Y())));
					streamers[streamerIndex++]->execute(entity);
				}
			}
		}

		float LODTerrainStreamer::getHeight(const Vector3& position) const
		{
			// TODO This assumes the position is within the highest resolution chunk.
			return streamers[0]->getHeight(position);
		}

		void LODTerrainStreamer::onAddEntity(Entity& entity)
		{
			for (unique_ptr<TerrainStreamer>& streamer : streamers)
			{
				streamer->onAddEntity(entity);
			}
		}

		void LODTerrainStreamer::setCenter(const Vector3& center)
		{
			worldPosition = center;
		}

		void LODTerrainStreamer::setTrackedEntity(const Entity& trackedEntity)
		{
			this->trackedEntity = &trackedEntity;
		}

		Vector2i LODTerrainStreamer::toLargestUnitPosition(const Vector3& worldPosition) const
		{
			return Vector2i(static_cast<int>(round(worldPosition.X() / largestUnitLength)),
								  static_cast<int>(round(worldPosition.Z() / largestUnitLength)));
		}
	}
}
