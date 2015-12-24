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
#ifndef RESOURCETERRAINSOURCE_H_
#define RESOURCETERRAINSOURCE_H_

#include <simplicity/resources/Resource.h>

#include "TerrainSource.h"

namespace simplicity
{
	namespace terrain
	{
		class ResourceTerrainSource : public TerrainSource
		{
			public:
				ResourceTerrainSource(const Vector2ui& mapSize, const Resource& resource,
									  unsigned int resourceOffset = 0);

				std::vector<float> getSectionHeights(const Vector2i& sectionNorthWest,
													 const Vector2i& sectionSize) const override;

				std::vector<Vector3> getSectionNormals(const Vector2i& sectionNorthWest,
													   const Vector2i& sectionSize) const override;

			private:
				Vector2ui mapSamples;

				unsigned int normalOffset;

				const Resource& resource;

				unsigned int resourceOffset;

				void readSection(const Vector2i& sectionNorthWest, const Vector2i& sectionSamples,
								 unsigned int offset, unsigned int stride, char* destination) const;

				Vector2i toResourceSpace(const Vector2i& position) const;
		};
	}
}

#endif /* RESOURCETERRAINSOURCE_H_ */
