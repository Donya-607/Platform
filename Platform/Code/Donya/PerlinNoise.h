#pragma once

namespace Donya
{
	// It depends on: https://solarianprogrammer.com/2012/07/18/perlin-noise-cpp-11/
	// "THIS CLASS IS A TRANSLATION TO C++11 FROM THE REFERENCE"
	// "JAVA IMPLEMENTATION OF THE IMPROVED PERLIN FUNCTION (see http://mrl.nyu.edu/~perlin/noise/)"
	// "THE ORIGINAL JAVA IMPLEMENTATION IS COPYRIGHT 2002 KEN PERLIN"
	// "I ADDED AN EXTRA METHOD THAT GENERATES A NEW PERMUTATION VECTOR (THIS IS NOT PRESENT IN THE ORIGINAL IMPLEMENTATION)"
	class PerlinNoise
	{
	private:
		static constexpr int valueCount = 256;
		int permutation[valueCount*2]{ 0 };
	public:
		// Construct by default permutation
		// "Initialize with the reference values for the permutation vector"
		PerlinNoise();
		// Construct by InitializeBySeed( seed )
		// "Generate a new permutation vector based on the value of seed"
		PerlinNoise( unsigned int seed );
		PerlinNoise( const PerlinNoise & ) = default;
		PerlinNoise(      PerlinNoise && ) = default;
		PerlinNoise &operator = ( const PerlinNoise & ) = default;
		PerlinNoise &operator = (      PerlinNoise && ) = default;
	public:
		// "Generate a new permutation vector based on the value of seed"
		void InitializeBySeed( unsigned int seed );
	public:
		// -1.0 ~ +1.0 as double
		// "Get a noise value, for 2D images z can have any value"
		double Noise( double x, double y, double z ) const;
		// -1.0 ~ +1.0 as float
		// "Get a noise value, for 2D images z can have any value"
		float Noise( float x, float y, float z ) const;
	private:
		double Fade( double t ) const;
		double Lerp( double t, double a, double b ) const;
		double Grad( int hash, double x, double y, double z ) const;
	};
}
