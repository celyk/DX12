namespace gfx{

	class Renderer{
	public:
		bool init(platform::Window* w);
		bool render();
		void shutdown();
	private:
		platform::Window* window;
	};

}