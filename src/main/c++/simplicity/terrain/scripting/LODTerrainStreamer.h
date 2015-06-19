#ifndef LODTERRAINSTREAMER_H_
#define LODTERRAINSTREAMER_H_

#include <simplicity/API.h>

#include "TerrainStreamer.h"

namespace simplicity
{
	namespace terrain
	{
		class LODTerrainStreamer : public Script
		{
			public:
				LODTerrainStreamer(const Resource& resource, const Vector2ui& mapSize,
								   const std::vector<Vector2ui>& chunkSizes,
								   const std::vector<unsigned int>& sampleFrequencies);

				void execute(Entity& entity) override;

				float getHeight(const Vector3& position) const;

				void onAddEntity(Entity& entity) override;

				void setCenter(const Vector3& center);

				void setTrackedEntity(const Entity& trackedEntity);

			private:
				vector<Vector2ui> chunkSizes;

				Vector2i currentPosition;

				bool initialized;

				float largestUnitLength;

				const Entity* trackedEntity;

				Vector3 worldPosition;

				std::vector<std::unique_ptr<TerrainStreamer>> streamers;

				Vector2i toLargestUnitPosition(const Vector3& worldPosition) const;
		};
	}
}

#endif /* LODTERRAINSTREAMER_H_ */
