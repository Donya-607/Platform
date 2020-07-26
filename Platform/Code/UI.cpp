#include "UI.h"

#include "Donya/Sprite.h"

bool UIObject::LoadSprite( const std::wstring &filePath, size_t maxCount )
{
	sprite = Donya::Sprite::Load( filePath, maxCount );
	return ( sprite == NULL ) ? false : true;
}

bool UIObject::Draw( float depth ) const
{
	const float prevDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( depth );

	const bool result =
	Donya::Sprite::DrawExt
	(
		sprite,
		pos.x,		pos.y,
		scale.x,	scale.y,
		degree,		origin,
		alpha,		color.x,	color.y,	color.z
	);

	Donya::Sprite::SetDrawDepth( prevDepth );
	return result;
}
bool UIObject::DrawPart( float depth ) const
{
	const float prevDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( depth );

	const bool result =
	Donya::Sprite::DrawPartExt
	(
		sprite,
		pos.x,		pos.y,
		texPos.x,	texPos.y,
		texSize.x,	texSize.y,
		scale.x,	scale.y,
		degree,		origin,
		alpha,		color.x,	color.y,	color.z
	);

	Donya::Sprite::SetDrawDepth( prevDepth );
	return result;
}

void UIObject::AssignSpriteID( size_t id )
{
	if ( id == NULL ) { return; }
	// else
	sprite = id;
}

Donya::Int2 UIObject::GetSpriteSize() const
{
	return Donya::Sprite::GetTextureSize( sprite );
}

#if USE_IMGUI
void UIObject::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat2	( u8"�X�N���[�����W",		&pos.x							);
	ImGui::SliderFloat2	( u8"���_�ʒu",			&origin.x,	0.0f,		1.0f	);

	ImGui::SliderFloat	( u8"�`��p�x",			&degree,	-360.0f,	360.0f	);
	ImGui::SliderFloat	( u8"�`��A���t�@",		&alpha,		0.0f,		1.0f	);
	ImGui::DragFloat2	( u8"�`��X�P�[��",		&scale.x,	0.1f				);
	ImGui::ColorEdit3	( u8"�u�����h�F",		&color.x						);

	ImGui::DragFloat2	( u8"�e�N�X�`���E������W",					&texPos.x	);
	ImGui::DragFloat2	( u8"�e�N�X�`���E�؂���T�C�Y�i�S�́j",		&texSize.x	);

	ImGui::TreePop();
}
#endif // USE_IMGUI
