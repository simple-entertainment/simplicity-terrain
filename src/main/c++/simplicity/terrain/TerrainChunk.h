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
				TerrainChunk(const Vector2ui& size,
							 const std::vector<unsigned int>& borderPatchLengths = { 1, 1, 1, 1 });

				std::unique_ptr<Mesh> createMesh();

				Mesh* getMesh() const;

				const Vector2ui& getSize() const;

			private:
				std::vector<unsigned int> borderPatchLengths;

				Mesh* mesh;

				Vector2ui size;

				unsigned int getBorderIndexCount(unsigned int borderLength, unsigned int borderPatchLength) const;

				void insertBorderIndices(MeshData& meshData);

				void insertBorderIndices(MeshData& meshData, unsigned int baseIndex, unsigned int baseVertex,
										 unsigned int borderLength, unsigned int patchLength, int borderUnitStride,
										 const std::vector<unsigned int>& vertexOrder);

				void insertInteriorIndices(MeshData& meshData, unsigned int baseIndex);
		};
	}
}

#endif /* TERRAINCHUNK_H_ */
