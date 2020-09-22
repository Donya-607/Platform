#pragma once

#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "Quaternion.h"
#include "Serializer.h"
#include "Vector.h"

#include "ModelCommon.h"

namespace Donya
{
	namespace Model
	{
		/// <summary>
		/// Store a data of model as it is.
		/// </summary>
		struct Source
		{
			struct Material
			{
				Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };	// RGBA.
				std::string		textureName;	// Relative file path. No need to multiple texture.
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP(	color		),
						CEREAL_NVP(	textureName	)
					);

					if ( 1 <= version )
					{
						// archive( CEREAL_NVP( x ) );
					}
				}
			};

			struct Subset
			{
				std::string		name;
				unsigned int	indexCount;
				unsigned int	indexStart;
				Material		ambient;
				Material		bump;
				Material		diffuse;
				Material		specular;
				Material		emissive;
				Material		normal;
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( name		),
						CEREAL_NVP(	indexCount	),
						CEREAL_NVP(	indexStart	),
						CEREAL_NVP( ambient		),
						CEREAL_NVP( bump		),
						CEREAL_NVP( diffuse		),
						CEREAL_NVP( specular	),
						CEREAL_NVP( emissive	)
					);

					if ( 1 <= version )
					{
						archive( CEREAL_NVP( normal ) );
					}
					if ( 2 <= version )
					{
						// archive( CEREAL_NVP( x ) );
					}
				}
			};

			/// <summary>
			/// The "Node" has many types. The Mesh is one of Node's types, the Bone is also one of the Node's types.
			/// </summary>
			struct Mesh
			{
				std::string						name;

				int								boneIndex;		// The index of associated skeletal with this mesh.
				std::vector<int>				boneIndices;	// The indices of associated skeletal with this mesh and this mesh's node. You can access to that associated skeletal with the index of "nodeIndices", like this: "currentPose = mesh.boneOffsets[i] * model.skeletal[boneIndices[i]].global;".
				std::vector<Animation::Node>	boneOffsets;	// Used as the bone-offset(inverse initial-pose) matrices of model's skeletal. You can access to that associated skeletal with the index of "nodeIndices", like this: "currentPose = mesh.boneOffsets[i] * model.skeletal[boneIndices[i]].global;".

				std::vector<Vertex::Pos>		positions;
				std::vector<Vertex::Tex>		texCoords;
				std::vector<Vertex::Bone>		boneInfluences;
				std::vector<unsigned int>		indices;		// A index list of vertices.
				std::vector<Subset>				subsets;
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP(	name			),
						CEREAL_NVP(	boneIndex		),
						CEREAL_NVP(	boneIndices		),
						CEREAL_NVP(	boneOffsets		),
						CEREAL_NVP(	positions		),
						CEREAL_NVP(	texCoords		),
						CEREAL_NVP(	boneInfluences	),
						CEREAL_NVP(	indices			),
						CEREAL_NVP(	subsets			)
					);
					
					if ( 1 <= version )
					{
						// archive( CEREAL_NVP( x ) );
					}
				}
			};
		public:
			std::vector<Mesh>				meshes;
			std::vector<Animation::Node>	skeletal;			// The model's skeletal of initial pose(so-called "T-pose"). 
			std::vector<Animation::Motion>	motions;			// Represent animations. The animations contain only animation(i.e. The animation matrix transforms space is bone -> mesh(current pose)).
			Donya::Vector4x4				coordinateConversion;
		public:
			// A posteriori adjustment in world space(i.e. in the space of transformed by "coordinateConversion")
			Donya::Vector4x4				extraTransform;
			Donya::Vector3					extraScale{ 1.0f, 1.0f, 1.0f };
			Donya::Quaternion				extraRotation;
			Donya::Vector3					extraTranslation{};
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP(	meshes		),
					CEREAL_NVP(	skeletal	),
					CEREAL_NVP(	motions		),
					CEREAL_NVP( coordinateConversion )
				);
				
				if ( 1 <= version )
				{
					archive
					(
						CEREAL_NVP( extraTransform		),
						CEREAL_NVP( extraScale			),
						CEREAL_NVP( extraRotation		),
						CEREAL_NVP( extraTranslation	)
					);
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
	}
}

CEREAL_CLASS_VERSION( Donya::Model::Source,				1 )
CEREAL_CLASS_VERSION( Donya::Model::Source::Subset,		1 )
CEREAL_CLASS_VERSION( Donya::Model::Source::Mesh,		0 )
CEREAL_CLASS_VERSION( Donya::Model::Source::Material,	0 )
