#ifndef TERRAINFACTORY_H_
#define TERRAINFACTORY_H_

#include <simplicity/math/Vector.h>
#include <simplicity/resources/Resource.h>

namespace simplicity
{
	namespace terrain
	{
		class TerrainFactory
		{
			public:
				using HeightFunction = float(int x, int y);

				static void createFlatTerrain(Resource& resource, const Vector2ui& mapSize,
											  std::function<HeightFunction> heightFunction,
											  const std::vector<unsigned int>& sampleFrequencies = { 1 });

			private:
				static float readHeight(std::istream& stream, const Vector2ui& mapSamples, unsigned int x,
										unsigned int y);

				static Vector3 readNormal(std::istream& stream, const Vector2ui& mapSamples, unsigned int x,
										unsigned int y);

				static void writeHighestFrequencySamples(Resource& resource, const Vector2ui& mapSamples,
														 std::function<HeightFunction> heightFunction,
														 unsigned int sampleFrequency);

				static void writeLowerFrequencySamples(Resource& resource, const Vector2ui& mapSamples,
													   const std::vector<unsigned int>& sampleFrequencies);
		};
	}
}

#endif /* TERRAINFACTORY_H_ */
