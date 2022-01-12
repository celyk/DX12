namespace gfx{

	class Renderer{
	public:
		bool init(platform::Window* w);
		void render();
		void shutdown();
	private:
		platform::Window* window;
	};

}