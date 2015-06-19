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
				TerrainStreamer(std::unique_ptr<TerrainSource> source, const Vector2ui& mapSize,
								const Vector2ui& chunkSize, float unitLength = 1.0f,
								const std::vector<unsigned int>& borderPatchLengths = { 1, 1, 1, 1 });

				void execute(Entity& entity) override;

				float getHeight(const Vector3& position) const;

				void onAddEntity(Entity& entity) override;

				void setCenter(const Vector3& center);

				void setTrackedEntity(const Entity& trackedEntity);

			private:
				TerrainChunk chunk;

				Vector2i currentPosition;

				bool initialized;

				Vector2ui mapSamples;

				Vector2i meshNorthWest;

				std::unique_ptr<TerrainSource> source;

				const Entity* trackedEntity;

				float unitLength;

				Vector3 worldPosition;

				Vector2i getChunkNorthWest() const;

				Vector2i meshMove(const Vector2i& position, const Vector2i& movement) const;

				void streamEastWest(const Vector2i& position, const Vector2i& movement,
									const Vector2ui& absoluteMovement);

				void streamNorthSouth(const Vector2i& position, const Vector2i& movement,
									  const Vector2ui& absoluteMovement);

				Vector2i toUnitPosition(const Vector3& worldPosition) const;

				void writeSection(const std::vector<float>& heightMap, const std::vector<Vector3>& normalMap,
								  unsigned int rows, unsigned int columns, const Vector2i& sectionNorthWest,
								  const Vector2i& meshNorthWest);

				void writeSquareVertices(MeshData& meshData, unsigned int vertexIndex, const Vector2& northWest,
										 const std::vector<float>& heights, const std::vector<Vector3>& normals,
										 const Vector4& color);
		};
	}
}

#endif /* TERRAINSTREAMER_H_ */
