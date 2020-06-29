#include "Terry.h"

#include "Parameter.h"

namespace Enemy
{
	namespace Parameter
	{
		static ParamOperator<TerryParam> terryParam{ "Terry" };
		const TerryParam &GetTerry()
		{
			return terryParam.Get();
		}

		namespace Impl
		{
			void LoadTerry()
			{
				terryParam.LoadParameter();
			}
		#if USE_IMGUI
			void UpdateTerry( const std::string &nodeCaption )
			{
				terryParam.ShowImGuiNode( nodeCaption );
			}
		#endif // USE_IMGUI
		}
	}

	void Terry::Init( const InitializeParam &parameter )
	{
		Base::Init( parameter );

		const auto &data = Parameter::GetTerry();
		body.offset		= data.hitBoxOffset;
		body.size		= data.hitBoxSize;
		hurtBox.offset	= data.hurtBoxOffset;
		hurtBox.size	= data.hurtBoxSize;
	}
	void Terry::Update( float elapsedTime, const Donya::Vector3 &wsTargetPos )
	{
		const auto &data = Parameter::GetTerry();

	#if USE_IMGUI
		// Apply for be able to see an adjustment immediately
		{
			body.offset		= data.hitBoxOffset;
			body.size		= data.hitBoxSize;
			hurtBox.offset	= data.hurtBoxOffset;
			hurtBox.size	= data.hurtBoxSize;
		}
	#endif // USE_IMGUI

		// Move
		{
			// HACK: Should consider the offset of myself's hit-box?
			const auto  toTarget	= wsTargetPos - body.WorldPosition();
			const float dist		= toTarget.Length();

			const float speed		= std::min( dist, data.moveSpeed );
			velocity = toTarget.Unit() * speed;
		}

		// Rotate
		{
			const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( data.rotateSpeed ) );
			orientation.RotateBy( rotation );
		}
	}
	Kind Terry::GetKind() const { return Kind::Terry; }
#if USE_IMGUI
	void Terry::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		Base::ShowImGuiNode( u8"��ꕔ��" );

		ImGui::TextDisabled( u8"�h������" );
		// if ( ImGui::TreeNode( u8"�h������" ) )
		// {
		// 
		// 	ImGui::TreePop();
		// }

		ImGui::TreePop();
	}
	void TerryParam::ShowImGuiNode()
	{
		ImGui::DragFloat3( u8"�����蔻��E�I�t�Z�b�g",			&hitBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"�����蔻��E�T�C�Y�i�������w��j",	&hitBoxSize.x,		0.01f );
		ImGui::DragFloat3( u8"��炢����E�I�t�Z�b�g",			&hurtBoxOffset.x,	0.01f );
		ImGui::DragFloat3( u8"��炢����E�T�C�Y�i�������w��j",	&hurtBoxSize.x,		0.01f );
		ImGui::DragFloat ( u8"�ړ����x",							&moveSpeed,			0.1f  );
		ImGui::DragFloat ( u8"��]���x[degree/s]",				&rotateSpeed,		1.0f  );
		hitBoxSize.x	= std::max( 0.0f,	hitBoxSize.x	);
		hitBoxSize.y	= std::max( 0.0f,	hitBoxSize.y	);
		hitBoxSize.z	= std::max( 0.0f,	hitBoxSize.z	);
		hurtBoxSize.x	= std::max( 0.0f,	hurtBoxSize.x	);
		hurtBoxSize.y	= std::max( 0.0f,	hurtBoxSize.y	);
		hurtBoxSize.z	= std::max( 0.0f,	hurtBoxSize.z	);
		moveSpeed		= std::max( 0.001f,	moveSpeed		);
	}
#endif // USE_IMGUI
}
