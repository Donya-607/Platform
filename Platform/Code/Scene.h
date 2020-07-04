#pragma once

class Scene
{
public:
	enum class Request : int
	{
		NONE				= 0,
		ADD_SCENE			= 1 << 0,	// Add a scene to front.
		APPEND_SCENE		= 1 << 1,	// Add a scene to back.
		REMOVE_ME			= 1 << 2,	// Remove the scene. If specify this request, you must be careful for the scene-stack do not empty.
		REMOVE_ALL			= 1 << 3,	// Remove all scene. If specify this request, you must be careful for the scene-stack do not empty.
		ASSIGN				= REMOVE_ALL | ADD_SCENE,	// Doing REMOVE_ALL and ADD_SCENE.
		UPDATE_NEXT			= 1 << 4,	// Also Update the next stacked scene.
	};
	enum class Type : int
	{
		Null = 0,
		Logo,
		Load,
		Title,
		Game,
		Result,
		Pause,
	};
	struct Result
	{
		Request	request		= Request::NONE;
		Type	sceneType	= Type::Null;
	public:
		Result() : request( Request::NONE ), sceneType( Type::Null ) {}
		Result( Request request, Type type ) : request( request ), sceneType( type ) {}
	public:
		void AddRequest( const Request &flag )
		{
			request = static_cast<Request>( static_cast<int>( request ) | static_cast<int>( flag ) );
		}
		void AddRequest( const Request &L, const Request &R )
		{
			AddRequest( L );
			AddRequest( R );
		}
	public:
		bool HasRequest( Request kind ) const
		{
			return
			( static_cast<int>( request ) & static_cast<int>( kind ) )
			? true
			: false;
		}
	};
public:
	Scene() {}
	virtual ~Scene() {}
public:
	virtual void	Init()			= 0;
	virtual void	Uninit()		= 0;
	virtual Result	Update( float elapsedTime )	= 0;
	virtual void	Draw( float elapsedTime )	= 0;
};
