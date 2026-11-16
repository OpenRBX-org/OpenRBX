#ifndef RENDERLIB_CLUSTERER_H
#define RENDERLIB_CLUSTERER_H

#include "renderlib/Chunk.h"

#include <G3D/Vector3.h>
#include <vector>

namespace RBX::Render {

class Clusterer
{
public:
	using Sample = Chunk;
	using Samples = std::vector<Sample*>;

	class Cluster
	{
	public:
		G3D::Vector3 centroid;
		Samples samples;
		unsigned int visitIndex;

		void computeCentroid();

		float getDistanceFromCentroid(Sample* sample) { return (sample->cframe().translation - centroid).length(); }
	};

	using Clusters = std::vector<Cluster>;

private:
	unsigned int sampleSize;
	const unsigned int clusterCount;
	Clusters clusters;

	unsigned int moveSamples();
	unsigned int moveSample(Sample* sample, Cluster* cluster);
	Cluster* findClosestCluster(Sample* sample);

public:
	Clusterer(unsigned int clusterCount);

	template <class Iterator>
	void addSamples(Iterator _Iter, Iterator _End)
	{
		unsigned int i = 0;
		for (; _Iter != _End; ++_Iter) {
			clusters[i].samples.push_back(&**_Iter);
			i = (i + 1) % clusterCount;
			sampleSize++;
		}
	}

	Clusters* go(unsigned int maxSteps);
};

} // namespace RBX::Render

#endif // RENDERLIB_CLUSTERER_H
