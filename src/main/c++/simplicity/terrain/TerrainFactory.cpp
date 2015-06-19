#include "TerrainFactory.h"

using namespace std;

namespace simplicity
{
	namespace terrain
	{
		void TerrainFactory::createFlatTerrain(Resource& resource, const Vector2ui& mapSize,
							   function<HeightFunction> heightFunction,
							   const vector<unsigned int>& sampleFrequencies)
		{
			Vector2ui mapSamples = mapSize;
			mapSamples.X() += 1;
			mapSamples.Y() += 1;

			writeHighestFrequencySamples(resource, mapSamples, heightFunction, sampleFrequencies[0]);

			if (sampleFrequencies.size() > 1)
			{
				writeLowerFrequencySamples(resource, mapSamples, sampleFrequencies);
			}
		}

		float TerrainFactory::readHeight(istream& stream, const Vector2ui& mapSamples, unsigned int x,
										 unsigned int y)
		{
			float height;

			stream.seekg((y * mapSamples.X() + x) * sizeof(float));
			stream.read(reinterpret_cast<char*>(&height), sizeof(float));

			return height;
		}

		Vector3 TerrainFactory::readNormal(istream& stream, const Vector2ui& mapSamples, unsigned int x,
										 unsigned int y)
		{
			Vector3 normal;

			unsigned int basePosition = mapSamples.X() * mapSamples.Y() * sizeof(float);
			stream.seekg(basePosition + (y * mapSamples.X() + x) * sizeof(float) * 3);
			stream.read(reinterpret_cast<char*>(normal.getData()), sizeof(float) * 3);

			return normal;
		}

		void TerrainFactory::writeHighestFrequencySamples(Resource& resource, const Vector2ui& mapSamples,
														  function<HeightFunction> heightFunction,
														  unsigned int sampleFrequency)
		{
			// Heights
			for (int x = 0; x < mapSamples.X(); x += sampleFrequency)
			{
				for (int y = 0; y < mapSamples.Y(); y += sampleFrequency)
				{
					float height = heightFunction(x, y);

					resource.appendData(reinterpret_cast<char*>(&height), sizeof(float));
				}
			}

			// Normals
			for (int x = 0; x < mapSamples.X(); x += sampleFrequency)
			{
				for (int y = 0; y < mapSamples.Y(); y += sampleFrequency)
				{
					float height = heightFunction(x, y);
					float heightN = heightFunction(x, y - sampleFrequency);
					float heightE = heightFunction(x + sampleFrequency, y);
					float heightS = heightFunction(x, y + sampleFrequency);
					float heightW = heightFunction(x - sampleFrequency, y);

					Vector3 point(0.0f, height, 0.0f);

					Vector3 edgeN = Vector3(0.0f, heightN, -1.0f) - point;
					edgeN.normalize();
					Vector3 edgeE = Vector3(1.0f, heightE, 0.0f) - point;
					edgeE.normalize();
					Vector3 edgeS = Vector3(0.0f, heightS, 1.0f) - point;
					edgeS.normalize();
					Vector3 edgeW = Vector3(-1.0f, heightW, 0.0f) - point;
					edgeW.normalize();

					Vector3 normal = crossProduct(edgeN, edgeW) + crossProduct(edgeW, edgeS) +
									 crossProduct(edgeS, edgeE) + crossProduct(edgeE, edgeN);
					normal.normalize();

					resource.appendData(reinterpret_cast<char*>(normal.getData()), sizeof(float) * 3);
				}
			}
		}

		void TerrainFactory::writeLowerFrequencySamples(Resource& resource, const Vector2ui& mapSamples,
														const vector<unsigned int>& sampleFrequencies)
		{
			unsigned int highestSampleFrequency = sampleFrequencies[0];
			unique_ptr<istream> stream = resource.getInputStream();

			for (unsigned int sample = 1; sample < sampleFrequencies.size(); sample++)
			{
				unsigned int sampleFrequencyRatio = sampleFrequencies[sample] / highestSampleFrequency;

				// Heights
				for (unsigned int row = 0; row < mapSamples.Y(); row += sampleFrequencyRatio)
				{
					for (unsigned int column = 0; column < mapSamples.X(); column += sampleFrequencyRatio)
					{
						float height = readHeight(*stream, mapSamples, column,row);
						resource.appendData(reinterpret_cast<char*>(&height), sizeof(float));
					}
				}

				// Normals
				for (unsigned int row = 0; row < mapSamples.Y(); row += sampleFrequencyRatio)
				{
					for (unsigned int column = 0; column < mapSamples.X(); column += sampleFrequencyRatio)
					{
						Vector3 normal = readNormal(*stream, mapSamples, column, row);
						resource.appendData(reinterpret_cast<char*>(normal.getData()), sizeof(float) * 3);
					}
				}
			}
		}
	}
}
