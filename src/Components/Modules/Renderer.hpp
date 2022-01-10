#pragma once

namespace Components
{
	class Renderer : public Component
	{
	public:
		typedef void(BackendCallback)(IDirect3DDevice9*);

		Renderer();
		~Renderer();

		static int Width();
		static int Height();

		static void OnBackendFrame(Utils::Slot<BackendCallback> callback);
		static void OnNextBackendFrame(Utils::Slot<BackendCallback> callback);
		static void OnDeviceRecoveryEnd(Utils::Slot<Scheduler::Callback> callback);
		static void OnDeviceRecoveryBegin(Utils::Slot<Scheduler::Callback> callback);


	private:
		static void FrameStub();

		static void BackendFrameStub();
		static void BackendFrameHandler();

		static void PreVidRestart();
		static void PostVidRestart();
		static void PostVidRestartStub();

		static void R_TextureFromCodeError(const char* sampler, Game::GfxCmdBufState* state);
		static void StoreGfxBufContextPtrStub1();
		static void StoreGfxBufContextPtrStub2();

		static int DrawTechsetForMaterial(int a1, float a2, float a3, const char* material, Game::vec4_t* color, int a6);

		static void DebugDrawTriggers();
		static void DebugDrawSceneModelCollisions();
		static void DebugDrawModelBoundingBoxes();
		static void DebugDrawModelNames();
		static void DebugDrawAABBTrees();

		static Utils::Signal<Scheduler::Callback> EndRecoverDeviceSignal;
		static Utils::Signal<Scheduler::Callback> BeginRecoverDeviceSignal;

		static Utils::Signal<BackendCallback> BackendFrameSignal;
		static Utils::Signal<BackendCallback> SingleBackendFrameSignal;

		static Dvar::Var r_drawTriggers;
		static Dvar::Var r_drawSceneModelCollisions;
		static Dvar::Var r_drawModelBoundingBoxes;
		static Dvar::Var r_drawModelNames;
		static Dvar::Var r_drawAABBTrees;
		static Dvar::Var r_playerDrawDebugDistance;
	};
}
