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
#include <simplicity/math/MathFunctions.h>
#include <simplicity/model/ModelFactory.h>

#include "TerrainChunk.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		TerrainChunk::TerrainChunk(unsigned int size, float scale) :
			model(nullptr),
			samples(size + 1),
			scale(scale),
			size(size)
		{
		}

		unique_ptr<Model> TerrainChunk::createModel()
		{
			unsigned int vertexCount = samples * samples;
			unsigned int squareCount = size * size;
			unsigned int indexCount = squareCount * 6;

			shared_ptr<MeshBuffer> terrainBuffer =
					ModelFactory::createMeshBuffer(vertexCount, indexCount, Buffer::AccessHint::READ_WRITE);
			shared_ptr<Mesh> mesh = shared_ptr<Mesh>(new Mesh(terrainBuffer));

			MeshData& meshData = mesh->getData(false);

			meshData.vertexCount = vertexCount;
			meshData.indexCount = indexCount;

			setIndices(meshData);

			mesh->releaseData();

			unique_ptr<Model> model = unique_ptr<Model>(new Model);
			model->setMesh(mesh);

			this->model = model.get();
			return move(model);
		}

		float TerrainChunk::getHeight(const Vector3& position) const
		{
			Vector2i meshPosition = getMeshPosition(position);

			if (meshPosition.X() < 0 || meshPosition.X() >= size ||
				meshPosition.Y() < 0 || meshPosition.Y() >= size)
			{
				return 0.0f;
			}

			float xLocal = fmod(position.X(), scale);
			float zLocal = fmod(position.Z(), scale);
			bool inFirstTriangle = xLocal < zLocal;

			const MeshData& meshData = model->getMesh()->getData();

			unsigned int baseVertexIndex = meshPosition.Y() * samples + meshPosition.X();
			Vector3 pointA(0.0f, meshData.vertexData[baseVertexIndex].position.Y(), 0.0f);
			Vector3 pointB;
			Vector3 pointC;
			if (inFirstTriangle)
			{
				pointB = Vector3(0.0f, meshData.vertexData[baseVertexIndex + samples].position.Y(), scale);
				pointC = Vector3(scale, meshData.vertexData[baseVertexIndex + samples + 1].position.Y(), scale);
			}
			else
			{
				pointB = Vector3(scale, meshData.vertexData[baseVertexIndex + samples + 1].position.Y(), scale);
				pointC = Vector3(scale, meshData.vertexData[baseVertexIndex + 1].position.Y(), 0.0f);
			}

			model->getMesh()->releaseData();

			Vector3 edge0 = pointB - pointA;
			Vector3 edge1 = pointC - pointA;
			Vector3 normal = crossProduct(edge0, edge1);
			normal.normalize();

			// Solve the equation for the plane (ax + by + cz + d = 0) to find y.
			float ax = normal.X() * xLocal;
			float b = normal.Y();
			float cz = normal.Z() * zLocal;
			float d = dotProduct(normal, pointA) * -1.0f;
			float y = (ax + cz + d) / b * -1.0f;

			return y;
		}

		Model* TerrainChunk::getModel()
		{
			return model;
		}

		const Model* TerrainChunk::getModel() const
		{
			return model;
		}

		unsigned int TerrainChunk::getSize() const
		{
			return size;
		}

		void TerrainChunk::patch(Edge edge, unsigned int patchSize)
		{
			MeshData& meshData = model->getMesh()->getData(false);

			if (edge == Edge::NORTH)
			{
				unsigned int patchCount = size / patchSize;

				for (unsigned int patchIndex = 0; patchIndex < patchCount; patchIndex++)
				{
					unsigned int baseIndex = patchIndex * patchSize * 6;
					unsigned int baseVertexIndex = patchIndex * patchSize;

					for (unsigned int unitIndex = 0; unitIndex < patchSize; unitIndex++)
					{
						if (unitIndex <= (patchSize - 1) / 2)
						{
							meshData.indexData[baseIndex + unitIndex * 6] = baseVertexIndex;

							// Collapse second triangle.
							meshData.indexData[baseIndex + unitIndex * 6 + 3] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 4] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 5] = baseVertexIndex;
						}
						else
						{
							meshData.indexData[baseIndex + unitIndex * 6] = baseVertexIndex + patchSize;

							// Collapse second triangle.
							meshData.indexData[baseIndex + unitIndex * 6 + 3] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 4] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 5] = baseVertexIndex;
						}

						if (unitIndex == (patchSize - 1) / 2)
						{
							meshData.indexData[baseIndex + unitIndex * 6 + 3] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 4] = baseVertexIndex + samples + unitIndex + 1;
							meshData.indexData[baseIndex + unitIndex * 6 + 5] = baseVertexIndex + patchSize;
						}
					}
				}
			}
			else if (edge == Edge::EAST)
			{
				unsigned int patchCount = size / patchSize;
				unsigned int indexOffset = (size - 1) * 6;
				unsigned int vertexOffset = samples - 2;

				for (unsigned int patchIndex = 0; patchIndex < patchCount; patchIndex++)
				{
					unsigned int baseIndex = indexOffset + size * patchIndex * patchSize * 6;
					unsigned int baseVertexIndex = vertexOffset + samples * patchIndex * patchSize;

					for (unsigned int unitIndex = 0; unitIndex < patchSize; unitIndex++)
					{
						if (unitIndex < patchSize / 2)
						{
							meshData.indexData[baseIndex + size * unitIndex * 6 + 2] = baseVertexIndex + 1;

							// Collapse second triangle.
							meshData.indexData[baseIndex + size * unitIndex * 6 + 3] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 4] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 5] = baseVertexIndex;
						}
						else
						{
							meshData.indexData[baseIndex + size * unitIndex * 6 + 2] = baseVertexIndex + samples * patchSize + 1;

							// Collapse second triangle.
							meshData.indexData[baseIndex + size * unitIndex * 6 + 3] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 4] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 5] = baseVertexIndex;
						}

						if (unitIndex == patchSize / 2)
						{
							meshData.indexData[baseIndex + size * unitIndex * 6 + 3] = baseVertexIndex + samples * unitIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 4] = baseVertexIndex + samples * patchSize + 1;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 5] = baseVertexIndex + 1;
						}
					}
				}
			}
			else if (edge == Edge::SOUTH)
			{
				unsigned int patchCount = size / patchSize;
				unsigned int indexOffset = size * (size - 1) * 6;
				unsigned int vertexOffset = samples * (size - 1);

				for (unsigned int patchIndex = 0; patchIndex < patchCount; patchIndex++)
				{
					unsigned int baseIndex = indexOffset + patchIndex * patchSize * 6;
					unsigned int baseVertexIndex = vertexOffset + patchIndex * patchSize;

					for (unsigned int unitIndex = 0; unitIndex < patchSize; unitIndex++)
					{
						if (unitIndex < patchSize / 2)
						{
							// Collapse first triangle.
							meshData.indexData[baseIndex + unitIndex * 6] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 1] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 2] = baseVertexIndex;

							meshData.indexData[baseIndex + unitIndex * 6 + 4] = baseVertexIndex + samples;
						}
						else
						{
							// Collapse first triangle.
							meshData.indexData[baseIndex + unitIndex * 6] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 1] = baseVertexIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 2] = baseVertexIndex;

							meshData.indexData[baseIndex + unitIndex * 6 + 4] = baseVertexIndex + samples + patchSize;
						}

						if (unitIndex == patchSize / 2)
						{
							meshData.indexData[baseIndex + unitIndex * 6] = baseVertexIndex + unitIndex;
							meshData.indexData[baseIndex + unitIndex * 6 + 1] = baseVertexIndex + samples;
							meshData.indexData[baseIndex + unitIndex * 6 + 2] = baseVertexIndex + samples + patchSize;
						}
					}
				}
			}
			else if (edge == Edge::WEST)
			{
				unsigned int patchCount = size / patchSize;

				for (unsigned int patchIndex = 0; patchIndex < patchCount; patchIndex++)
				{
					unsigned int baseIndex = size * patchIndex * patchSize * 6;
					unsigned int baseVertexIndex = samples * patchIndex * patchSize;

					for (unsigned int unitIndex = 0; unitIndex < patchSize; unitIndex++)
					{
						if (unitIndex <= (patchSize - 1) / 2)
						{
							// Collapse first triangle.
							meshData.indexData[baseIndex + size * unitIndex * 6] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 1] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 2] = baseVertexIndex;

							meshData.indexData[baseIndex + size * unitIndex * 6 + 3] = baseVertexIndex;
						}
						else
						{
							// Collapse first triangle.
							meshData.indexData[baseIndex + size * unitIndex * 6] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 1] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 2] = baseVertexIndex;

							meshData.indexData[baseIndex + size * unitIndex * 6 + 3] = baseVertexIndex + samples * patchSize;
						}

						if (unitIndex == (patchSize - 1) / 2)
						{
							meshData.indexData[baseIndex + size * unitIndex * 6] = baseVertexIndex;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 1] = baseVertexIndex + samples * patchSize;
							meshData.indexData[baseIndex + size * unitIndex * 6 + 2] = baseVertexIndex + samples * (unitIndex + 1) + 1;
						}
					}
				}
			}

			model->getMesh()->releaseData();
		}

		void TerrainChunk::setIndices(MeshData& meshData)
		{
			unsigned int indexIndex = 0;

			for (unsigned int row = 0; row < size; row++)
			{
				for (unsigned int column = 0; column < size; column++)
				{
					unsigned int baseVertexIndex = row * samples + column;

					meshData.indexData[indexIndex++] = baseVertexIndex;
					meshData.indexData[indexIndex++] = baseVertexIndex + samples;
					meshData.indexData[indexIndex++] = baseVertexIndex + samples + 1;
					meshData.indexData[indexIndex++] = baseVertexIndex;
					meshData.indexData[indexIndex++] = baseVertexIndex + samples + 1;
					meshData.indexData[indexIndex++] = baseVertexIndex + 1;
				}
			}
		}

		void TerrainChunk::setVertices(const Vector2i& mapNorthWest, const vector<float>& heightMap,
									   const vector<Vector3>& normalMap)
		{
			MeshData& meshData = model->getMesh()->getData(false);

			for (unsigned int row = 0; row < samples; row++)
			{
				for (unsigned int column = 0; column < samples; column++)
				{
					Vertex& vertex = meshData.vertexData[row * samples + column];

					vertex.normal = normalMap[row * samples + column];

					vertex.position.X() = static_cast<float>(mapNorthWest.X()) + static_cast<float>(column) * scale;
					vertex.position.Y() = heightMap[row * samples + column];
					vertex.position.Z() = static_cast<float>(mapNorthWest.Y()) + static_cast<float>(row) * scale;

					// Random
					//vertex.color = Vector4(getRandomInt(0, 1), getRandomInt(0, 1), getRandomInt(0, 1), 1.0f);

					// Grass
					vertex.color = Vector4(0.0f, 0.5f, 0.0f, 1.0f);

					// Snow
					if (vertex.position.Y() > 60.0f)
					{
						vertex.color = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
					}

					// Sand
					if (vertex.position.Y() < 2.0f)
					{
						vertex.color = Vector4(0.83f, 0.65f, 0.15f, 1.0f);
					}

					// White borders
					/*if (row == 0 ||
						row == size ||
						column == 0 ||
						column == size)
					{
						vertex.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
					}*/
				}
			}

			model->getMesh()->releaseData();
		}

		Vector2i TerrainChunk::getMeshPosition(const Vector3& worldPosition) const
		{
			return Vector2i(static_cast<int>(floor(worldPosition.X() / scale)),
							static_cast<int>(floor(worldPosition.Z() / scale)));
		}
	}
}
