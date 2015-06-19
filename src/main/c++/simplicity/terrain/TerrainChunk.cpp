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
#include <simplicity/model/ModelFactory.h>

#include "TerrainChunk.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		TerrainChunk::TerrainChunk(const Vector2ui& size, const vector<unsigned int>& borderPatchLengths) :
				borderPatchLengths(borderPatchLengths),
				mesh(nullptr),
				size(size)
		{
		}

		unique_ptr<Mesh> TerrainChunk::createMesh()
		{
			unsigned int squareCount = size.X() * size.Y();
			unsigned int vertexCount = squareCount * 4;
			unsigned int borderIndexCount = getBorderIndexCount(size.X(), borderPatchLengths[0]) +
					getBorderIndexCount(size.Y(), borderPatchLengths[1]) +
					getBorderIndexCount(size.X(), borderPatchLengths[2]) +
					getBorderIndexCount(size.Y(), borderPatchLengths[3]);
			unsigned int indexCount = borderIndexCount + (squareCount - size.X() - size.Y() + 1) * 6;

			shared_ptr<MeshBuffer> terrainBuffer =
					ModelFactory::getInstance()->createMeshBuffer(vertexCount, indexCount,
																  Buffer::AccessHint::READ_WRITE);
			unique_ptr<Mesh> mesh = unique_ptr<Mesh>(new Mesh(terrainBuffer));

			MeshData& meshData = mesh->getData(false);

			meshData.vertexCount = vertexCount;
			meshData.indexCount = indexCount;

			insertBorderIndices(meshData);
			insertInteriorIndices(meshData, borderIndexCount);

			mesh->releaseData();

			this->mesh = mesh.get();
			return move(mesh);
		}

		unsigned int TerrainChunk::getBorderIndexCount(unsigned int borderLength,
															unsigned int borderPatchLength) const
		{
			return (borderLength - 2 + borderLength / borderPatchLength) * 3;
		}

		Mesh* TerrainChunk::getMesh() const
		{
			return mesh;
		}

		const Vector2ui& TerrainChunk::getSize() const
		{
			return size;
		}

		void TerrainChunk::insertBorderIndices(MeshData& meshData)
		{
			unsigned int baseIndex = 0;
			unsigned int squareCount = size.X() * size.Y();

			// North Border
			insertBorderIndices(meshData, baseIndex, 0, size.X(), borderPatchLengths[0], 4, {0, 1, 2, 3});
			baseIndex += getBorderIndexCount(size.X(), borderPatchLengths[0]);

			// East Border
			insertBorderIndices(meshData, baseIndex, (size.X() - 1) * 4, size.Y(), borderPatchLengths[1], size.X() * 4,
								{3, 0, 1, 2});
			baseIndex += getBorderIndexCount(size.Y(), borderPatchLengths[1]);

			// South Border
			insertBorderIndices(meshData, baseIndex, (squareCount - 1) * 4, size.X(), borderPatchLengths[2], -4,
								{2, 3, 0, 1});
			baseIndex += getBorderIndexCount(size.X(), borderPatchLengths[2]);

			// West Border
			insertBorderIndices(meshData, baseIndex, (squareCount - size.X()) * 4, size.Y(), borderPatchLengths[3],
								size.X() * -4, {1, 2, 3, 0});
		}

		void TerrainChunk::insertBorderIndices(MeshData& meshData, unsigned int baseIndex, unsigned int baseVertex,
											   unsigned int borderLength, unsigned int patchLength,
											   int borderUnitStride, const vector<unsigned int>& vertexOrder)
		{
			unsigned int indexIndex = baseIndex;
			unsigned int centerPatchIndex = patchLength / 2;

			for (unsigned int borderUnitIndex = 0; borderUnitIndex < borderLength;
				 borderUnitIndex += patchLength)
			{
				unsigned int firstBorderUnitIndex = baseVertex +
													borderUnitIndex * borderUnitStride;
				unsigned int centerBorderUnitIndex = baseVertex +
													 (borderUnitIndex + centerPatchIndex) * borderUnitStride;
				unsigned int lastBorderUnitIndex = baseVertex +
												   (borderUnitIndex + patchLength - 1) * borderUnitStride;

				// The special case where the patch length is 1 (i.e. no patching is really needed...). Luckily this
				// algorithm mostly applies correct indices for it except for the very first unit where the center index
				// needs to be moved.
				if (borderUnitIndex == 0 && patchLength == 1)
				{
					centerBorderUnitIndex += borderUnitStride;
				}

				meshData.indexData[indexIndex++] = firstBorderUnitIndex + vertexOrder[0];
				meshData.indexData[indexIndex++] = centerBorderUnitIndex + vertexOrder[1];
				meshData.indexData[indexIndex++] = lastBorderUnitIndex + vertexOrder[3];

				for (unsigned int borderUnit = 0; borderUnit < patchLength; borderUnit++)
				{
					if ((borderUnitIndex == 0 && borderUnit == 0) ||
						(borderUnitIndex == borderLength - patchLength && borderUnit == patchLength - 1))
					{
						continue;
					}

					unsigned int currentBorderUnitIndex = firstBorderUnitIndex + borderUnit * borderUnitStride;

					if (borderUnit < centerPatchIndex)
					{
						meshData.indexData[indexIndex++] = firstBorderUnitIndex + vertexOrder[0];
					}
					else
					{
						meshData.indexData[indexIndex++] = lastBorderUnitIndex + vertexOrder[3];
					}

					meshData.indexData[indexIndex++] = currentBorderUnitIndex + vertexOrder[1];
					meshData.indexData[indexIndex++] = currentBorderUnitIndex + vertexOrder[2];
				}
			}
		}

		void TerrainChunk::insertInteriorIndices(MeshData& meshData, unsigned int baseIndex)
		{
			unsigned int vertexIndex = 0;
			unsigned int indexIndex = baseIndex;
			for (unsigned int row = 0; row < size.Y(); row++)
			{
				if (row < 1 || row >= size.Y() - 1)
				{
					vertexIndex += size.X() * 4;
					continue;
				}

				for (unsigned int column = 0; column < size.X(); column++)
				{
					if (column < 1 || column >= size.X() - 1)
					{
						vertexIndex += 4;
						continue;
					}

					ModelFactory::insertRectangleIndices(meshData.indexData, indexIndex, vertexIndex);

					vertexIndex += 4;
					indexIndex += 6;
				}
			}
		}
	}
}
