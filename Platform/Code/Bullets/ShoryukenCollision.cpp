#include "ShoryukenCollision.h"

#include "../Donya/RenderingStates.h"
#include "../Donya/Sound.h"

#include "../Common.h"
#include "../Music.h"
#include "../Parameter.h"

namespace Bullet
{
	namespace Parameter
	{
		static ParamOperator<ShoryuColParam> shoryuColParam{ "ShoryukenCollision", "Bullet/" };
		const ShoryuColParam &GetShoryuCol()
		{
			return shoryuColParam.Get();
		}

		namespace Impl
		{
			void LoadShoryuCol()
			{
				shoryuColParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateShoryuCol( const std::string &nodeCaption )
			{
				shoryuColParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void ShoryuCol::Init( const FireDesc &parameter )
	{
		Base::Init( parameter );
	}
	void ShoryuCol::Uninit() {} // No op
	void ShoryuCol::Draw( RenderingHelper *pRenderer ) const
	{
		// No op
	}
	Kind ShoryuCol::GetKind() const
	{
		return Kind::ShoryukenCollision;
	}
	void ShoryuCol::GenerateCollidedEffect() const
	{
		// No op
	}
	void ShoryuCol::PlayCollidedSE() const
	{
		// No op
	}
	Definition::Damage ShoryuCol::GetDamageParameter() const
	{
		return Parameter::GetShoryuCol().basic.damage;
	}
	void ShoryuCol::AssignBodyParameter( const Donya::Vector3 &wsPos )
	{
		const auto &data = Parameter::GetShoryuCol().basic;
		body.pos	= wsPos;
		body.offset	= orientation.RotateVector( data.hitBoxOffset );
		body.size	= data.hitBoxSize;
	}
#if USE_IMGUI
	void ShoryuColParam::ShowImGuiNode()
	{
		basic.ShowImGuiNode( u8"îƒópê›íË" );
	}
#endif // USE_IMGUI
}
