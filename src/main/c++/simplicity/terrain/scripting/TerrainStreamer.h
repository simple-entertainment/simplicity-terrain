#ifndef TERRAINSTREAMER_H_
#define TERRAINSTREAMER_H_

#include <simplicity/model/Mesh.h>
#include <simplicity/scripting/Script.h>

#include "../LevelOfDetail.h"
#include "../TerrainChunk.h"
#include "../TerrainSource.h"

namespace simplicity
{
	namespace terrain
	{
		class TerrainStreamer : public Script
		{
			public:
				TerrainStreamer(std::unique_ptr<TerrainSource> source, const Vector2ui& mapSize,
								unsigned int chunkSize, const std::vector<LevelOfDetail>& lods = {});

				void execute() override;

				float getHeight(const Vector3& position) const;

				void onAddEntity() override;

				void setTarget(const Entity& target);

				void setTarget(const Vector3& target);

			private:
				std::vector<std::vector<TerrainChunk>> chunks;

				unsigned int chunkSize;

				std::map<unsigned int, unsigned int> layerMap;

				std::vector<LevelOfDetail> lods;

				Vector2ui mapSize;

				Vector2ui northWestChunk;

				Vector2i mapNorthWest;

				Vector2i mapSouthEast;

				Vector3 northWestPosition;

				unsigned int radius;

				unsigned int size;

				std::unique_ptr<TerrainSource> source;

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
