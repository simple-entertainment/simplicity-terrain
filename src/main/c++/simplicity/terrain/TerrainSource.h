#ifndef TERRAINSOURCE_H_
#define TERRAINSOURCE_H_

#include <simplicity/math/Vector.h>

namespace simplicity
{
	namespace terrain
	{
		class TerrainSource
		{
			public:
				virtual std::vector<float> getSectionHeights(const Vector2i& sectionNorthWest,
															 const Vector2i& sectionSize) const = 0;

				virtual std::vector<Vector3> getSectionNormals(const Vector2i& sectionNorthWest,
															   const Vector2i& sectionSize) const = 0;
		};
	}
}

#endif /* TERRAINSOURCE_H_ */
