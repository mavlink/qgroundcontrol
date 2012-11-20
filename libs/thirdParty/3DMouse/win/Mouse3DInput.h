#ifndef T3DMOUSE_INPUT_H
#define T3DMOUSE_INPUT_H

#include "MouseParameters.h"

#include <QWidget>
#include <vector>
#include <map>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  //target at least windows XP
#endif

#include <windows.h>


/*
	A class for connecting to and receiving data from a 3D Connexion 3D mouse

	This helper class manages the connection to a 3D mouse,  collecting WM_INPUT
	messages from Windows and converting the data into 3D motion data.

	This class is based on the SDK from 3D Connexion but is modified to work with Qt.

	It is Windows only since it uses the WM_INPUT messages from windows directly
	rather than Qt events.

	Note that it needs to be compiled with _WIN32_WINNT defined as 0x0501 (windows XP)
	or later because the raw input API was added in Windows XP. This also means that
	Qt needs to be compiled with this enabled otherwise the QEventDispatcherWin32 blocks
	in processEvents because the raw input messages do not cause the thread to be woken if
	Qt is compiled for Win 2000 targets.
*/

class Mouse3DInput : public QObject
{
	Q_OBJECT
public:
		Mouse3DInput(QWidget* widget);
		~Mouse3DInput();

static	bool		Is3dmouseAttached();

		I3dMouseParam&			MouseParams();
		const I3dMouseParam&	MouseParams() const;

virtual	void		Move3d(HANDLE device, std::vector<float>& motionData);
virtual void		On3dmouseKeyDown(HANDLE device, int virtualKeyCode);
virtual void		On3dmouseKeyUp(HANDLE device, int virtualKeyCode);

signals:

		void		Move3d(std::vector<float>& motionData);
		void		On3dmouseKeyDown(int virtualKeyCode);
		void		On3dmouseKeyUp(int virtualKeyCode);

private:

		bool		InitializeRawInput(HWND hwndTarget);

static	bool		RawInputEventFilter(void* msg, long* result);

		void		OnRawInput(UINT nInputCode, HRAWINPUT hRawInput);
		UINT		GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
		bool		TranslateRawInputData(UINT nInputCode, PRAWINPUT pRawInput);
		void		On3dmouseInput();

		class TInputData
		{
		public:
		   TInputData() : fAxes(6) {}

		   bool IsZero() {
			  return (0.==fAxes[0] && 0.==fAxes[1] && 0.==fAxes[2] &&
				 0.==fAxes[3] && 0.==fAxes[4] && 0.==fAxes[5]);
		   }

		   int                   fTimeToLive; // For telling if the device was unplugged while sending data
		   bool                  fIsDirty;
		   std::vector<float>	 fAxes;

		};

		HWND								fWindow;

		// Data cache to handle multiple rawinput devices
		std::map< HANDLE, TInputData>       fDevice2Data;
		std::map< HANDLE, unsigned long>    fDevice2Keystate;

		// 3dmouse parameters
		MouseParameters						f3dMouseParams;     // Rotate, Pan Zoom etc.

		// use to calculate distance traveled since last event
		DWORD								fLast3dmouseInputTime;

};

#endif
