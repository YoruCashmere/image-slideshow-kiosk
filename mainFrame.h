#pragma once
#include<filesystem>
#include <wx/glcanvas.h>
#include<wx/stdpaths.h>
#include<vector>
#include<wx/wx.h>
#include<wx/msw/registry.h>
#include<string>
#include<fstream>
#include<wx/dc.h>
#include<time.h>
#include<random>
#include<algorithm>
#include<chrono>

class mainFrame:public wxFrame
{
public:
	mainFrame(const wxString &title);
private:
	enum {
		ID_OPEN = wxID_HIGHEST + 1,
		ID_CLOSE
	};
	wxTimer* timer;
	int currentIndex = 0,count=0;
	std::vector<std::string>Imagepaths;
	wxMenuBar* menuBar;
	wxPanel* mainPanel;
	wxBoxSizer* mainPS;
	//define all thinks and set then to null for memory security
	wxGLCanvas* glCanvas = nullptr;						//literally the canvas like a window u draw in
	wxGLContext* glContext = nullptr;					//the brain 
	GLuint textureID;									//store state
	bool isDragging = true;
	wxPoint dragStart;
	std::string configPath;

	bool isResizing = false;
	void removeTaskbar();
	void createControlsMP();
	void uploadImage(const std::string& imagePath);
	void bindContolsMP();
	void createMenuBar();
	std::string getImagePath();
	void checkImagePath();
	void registerStartup();
	void openFile(wxCommandEvent& evt);
	void closeWindow(wxCommandEvent& evt);
	void pickUpFiles(const std::string& folderPath);
	void fireTimer(wxTimerEvent& evt);
	void OnPaint(wxPaintEvent& evt);
	void onShow(wxShowEvent& evt);
	void resize(wxSizeEvent& evt);
	void swapp(wxMouseEvent& evt);

};

