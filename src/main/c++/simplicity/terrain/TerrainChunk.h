/*
 * Copyright © 2015 Simple Entertainment Limited
 *
 * This file is part of The Simplicity Engine.
 *
 * The Simplicity Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * The Simplicity Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with The Simplicity Engine. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef TERRAINCHUNK_H_
#define TERRAINCHUNK_H_

#include <simplicity/model/Mesh.h>

namespace simplicity
{
	namespace terrain
	{
		class TerrainChunk
		{
			public:
				enum class Edge
				{
					EAST,
					NORTH,
					SOUTH,
					WEST
				};

				TerrainChunk(unsigned int size, float scale = 1.0f);

				std::unique_ptr<Mesh> createMesh();

				float getHeight(const Vector3& position) const;

				Mesh* getMesh();

				const Mesh* getMesh() const;

				Vector2i getMeshPosition(const Vector3& worldPosition) const;

				unsigned int getSize() const;

				void patch(Edge edge, unsigned int patchSize);

				void setVertices(const Vector2i& mapNorthWest, const std::vector<float>& heightMap,
								 const std::vector<Vector3>& normalMap);

			private:
				Mesh* mesh;

				unsigned int samples;

				float scale;

				unsigned int size;

				void setIndices(MeshData& meshData);
		};
	}
}

#endif /* TERRAINCHUNK_H_ */
