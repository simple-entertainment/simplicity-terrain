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

				void onAddEntity() override;

				void setTarget(const Entity& target);

				void setTarget(const Vector3& target);

			private:
				Vector2i centerPosition;

				std::vector<std::vector<TerrainChunk>> chunks;

				unsigned int chunkSize;

				std::map<unsigned int,unsigned int> layerMap;

				std::unique_ptr<std::vector<LevelOfDetail>> levelsOfDetail;

				Vector2ui mapSize;

				Vector2ui northWestChunk;

				float radiusf;

				unsigned int radiusi;

				const Entity* targetEntity;

				Vector3 targetPosition;

				void stream(const Vector2i& movement);
		};
	}
}

#endif /* TERRAINSTREAMER_H_ */
