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
#include "ResourceTerrainSource.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		const unsigned int HEIGHT_STRIDE = sizeof(float);

		const unsigned int NORMAL_STRIDE = sizeof(float) * 3;

		ResourceTerrainSource::ResourceTerrainSource(const Vector2ui& mapSize, const Resource& resource,
													 const vector<LevelOfDetail>& lods):
			heightOffsets(),
			lods(lods),
			mapSize(mapSize),
			normalOffsets(),
			resource(resource)
		{
			if (lods.size() == 0)
			{
				heightOffsets.push_back(0);

				unsigned int mapSamples = (mapSize.X() + 1) * (mapSize.Y() + 1);
				normalOffsets.push_back(mapSamples * HEIGHT_STRIDE);
			}
			else
			{
				unsigned int offset = 0;
				for (LevelOfDetail lod : lods)
				{
					Vector2ui lodSize = mapSize / lod.sampleFrequency;
					unsigned int lodSamples = (lodSize.X() + 1) * (lodSize.Y() + 1);

					heightOffsets.push_back(offset);
					offset += lodSamples * HEIGHT_STRIDE;
					normalOffsets.push_back(offset);
					offset += lodSamples * NORMAL_STRIDE;
				}
			}
		}

		vector<float> ResourceTerrainSource::getSectionHeights(const Vector2i& sectionNorthWest,
															   const Vector2ui& sectionSize,
															   unsigned int lodIndex) const
		{
			Vector2ui sectionSamples(sectionSize.X() + 1, sectionSize.Y() + 1);

			vector<float> heightMap(static_cast<size_t>(sectionSamples.X() * sectionSamples.Y()));

			readSection(sectionNorthWest, sectionSamples, lodIndex, heightOffsets[lodIndex], HEIGHT_STRIDE,
						reinterpret_cast<char*>(heightMap.data()));

			return heightMap;
		}

		vector<Vector3> ResourceTerrainSource::getSectionNormals(const Vector2i& sectionNorthWest,
																 const Vector2ui& sectionSize,
																 unsigned int lodIndex) const
		{
			Vector2ui sectionSamples(sectionSize.X() + 1, sectionSize.Y() + 1);

			vector<Vector3> normalMap(static_cast<size_t>(sectionSamples.X() * sectionSamples.Y()));

			readSection(sectionNorthWest, sectionSamples, lodIndex, normalOffsets[lodIndex], NORMAL_STRIDE,
						reinterpret_cast<char*>(normalMap.data()));

			return normalMap;
		}

		void ResourceTerrainSource::readSection(const Vector2i& sectionNorthWest, const Vector2ui& sectionSamples,
												unsigned int lodIndex, unsigned int offset, unsigned int stride,
												char* destination) const
		{
			Vector2ui lodSize = mapSize / lods[lodIndex].sampleFrequency;

			unsigned int resourceRowSize = (lodSize.X() + 1) * stride;
			unsigned int sectionRowSize = sectionSamples.X() * stride;

			Vector2i resourceNorthWest = toResourceSpace(lodIndex, sectionNorthWest);
			unsigned int resourcePosition = offset +
					resourceNorthWest.Y() * resourceRowSize + resourceNorthWest.X() * stride;
			unique_ptr<istream> resourceStream = resource.getInputStream();

			for (unsigned int row = 0; row < sectionSamples.Y(); row++)
			{
				resourceStream->seekg(resourcePosition);
				resourceStream->read(&destination[row * sectionRowSize], sectionRowSize);

				resourcePosition += resourceRowSize;
			}
		}

		Vector2i ResourceTerrainSource::toResourceSpace(unsigned int lodIndex, const Vector2i& position) const
		{
			Vector2ui lodSize = mapSize / lods[lodIndex].sampleFrequency;

			Vector2i resourcePosition = position;
			resourcePosition.X() += (lodSize.X() + 1) / 2;
			resourcePosition.Y() += (lodSize.Y() + 1) / 2;

			return resourcePosition;
		}
	}
}
