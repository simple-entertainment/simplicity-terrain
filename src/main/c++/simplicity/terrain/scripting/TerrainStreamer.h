#ifndef TERRAINSTREAMER_H_
#define TERRAINSTREAMER_H_

#include <simplicity/model/Mesh.h>
#include <simplicity/scripting/Script.h>

#include "../TerrainChunk.h"
#include "../TerrainSource.h"

namespace simplicity
{
	namespace terrain
	{
		class TerrainStreamer : public Script
		{
			public:
				struct LevelOfDetail
				{
					unsigned int layerCount;

					unsigned int sampleFrequency;

					std::unique_ptr<TerrainSource> source;
				};

				TerrainStreamer(std::unique_ptr<std::vector<LevelOfDetail>> levelsOfDetail, const Vector2ui& mapSize,
								unsigned int chunkSize);

				void execute() override;

				float getHeight(const Vector3& position) const;

				void onAddEntity() override;

				void setTarget(const Entity& target);

				void setTarget(const Vector3& target);

			private:
				std::vector<std::vector<TerrainChunk>> chunks;

				unsigned int chunkSize;

				std::map<unsigned int,unsigned int> layerMap;

				std::unique_ptr<std::vector<LevelOfDetail>> levelsOfDetail;

				Vector2ui mapSize;

				Vector2ui northWestChunk;

				Vector3 northWestPosition;

				unsigned int radius;

				unsigned int size;

				const Entity* targetEntity;

				Vector3 targetPosition;

				void stream(const Vector2i& movement);

				Vector2i toChunkPosition(const Vector3& position) const;

				Vector3 toRelativePosition(const Vector3& position, bool relativeToCenter = false) const;

				Vector3 toWorldPosition(const Vector2i& position) const;
		};
	}
}

#endif /* TERRAINSTREAMER_H_ */
