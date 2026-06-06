#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#include"stb_image.h"
#include "mainFrame.h"
#include<filesystem>
//randomise the vector first before starting to pick the images
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine  engine(seed);

mainFrame::mainFrame(const wxString& title)			//wxFRAME_NO_TASKBAR,wxBORDER_NONE OR wxNO_BORDER are use to remove task bars
	:wxFrame(nullptr,wxID_ANY,title,wxDefaultPosition,wxDefaultSize, wxFRAME_NO_TASKBAR|wxBORDER_NONE)
{

	timer = new wxTimer(this);
	createControlsMP();
	bindContolsMP();
	createMenuBar();
	wxSize sz = wxGetDisplaySize();	
	wxSize winSz =this->GetSize();
	
	CallAfter([this]() {
		wxSize sz = wxGetDisplaySize();
		wxSize winSz = this->GetSize();
		Move(sz.x - (winSz.x + 10), sz.y - (winSz.y+50));
		});

#ifdef __WXMSW__
	this->registerStartup();
#endif 
}

void mainFrame::removeTaskbar()
{
	long style = GetWindowStyle();
	style |= wxFRAME_NO_TASKBAR;
	style |= wxNO_BORDER;
	SetWindowStyleFlag(style);
	this->Refresh();
}

void mainFrame::createControlsMP()
{
	int glAttrib[] = { WX_GL_RGBA,WX_GL_DOUBLEBUFFER,
					WX_GL_DEPTH_SIZE,16,0 };							//configuration to set up glCanvas 
	glCanvas = new wxGLCanvas(this, wxID_ANY, glAttrib);
	glContext = new wxGLContext(glCanvas);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(glCanvas, wxSizerFlags().Proportion(1).Expand());
	SetSizer(sizer);
}
void mainFrame::uploadImage(const std::string& imagePath)
{
	glCanvas->SetCurrent(*glContext);
	int width, height, channels;											//variables to store the data that will be gathered from the image
	stbi_set_flip_vertically_on_load(true);									//opengl cordinate start from buttom left while most image cordinate starts from topleft
	unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);			//loading and collecting data about the images
	//delete curropted files from the vector
	if (!data) {
		Imagepaths.erase(Imagepaths.begin() + currentIndex);
		if (!Imagepaths.empty()) {
			currentIndex = currentIndex % Imagepaths.size();				//skip the file and use recursion to call the file
			if (count < 3) {
				uploadImage(Imagepaths[currentIndex]);
				count++;
			}
			else {
				timer->Stop();
				textureID = 0;
				glCanvas->Refresh();
				wxMessageBox("Some file in this folder are corrupted", "Curropted Files", wxOK | wxICON_ERROR);
				count = 0;
			}				
		}
		return;
	}
	if (textureID)glDeleteTextures(1, &textureID);							//free gpu memory
	glGenTextures(1, &textureID);											//create a slot in the gpu and returns its id
	glBindTexture(GL_TEXTURE_2D, textureID);								//make texture the default and tells the gpu its a 2d
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		//control scaling
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA
		, GL_UNSIGNED_BYTE, data);											//loads pixel from ram to gpu ones all is well					
	stbi_image_free(data);
	glCanvas->Refresh();	
}

void mainFrame::bindContolsMP()
{
	this->Bind(wxEVT_SHOW, &mainFrame::onShow, this);
	this->Bind(wxEVT_MENU, &mainFrame::openFile, this,ID_OPEN);
	this->Bind(wxEVT_TIMER, &mainFrame::fireTimer, this);
	this->Bind(wxEVT_MENU, &mainFrame::closeWindow, this,ID_CLOSE);
	glCanvas->Bind(wxEVT_LEFT_DCLICK, &mainFrame::swapp, this);
	glCanvas->Bind(wxEVT_PAINT, &mainFrame::OnPaint, this);
	glCanvas->Bind(wxEVT_SIZE, &mainFrame::resize, this);
}

void mainFrame::createMenuBar()
{
	menuBar = new wxMenuBar();
	wxMenu *Aple = new wxMenu();
	menuBar->Append(Aple, _("&Aple"));
	Aple->Append(ID_OPEN, _("&Open\tCtrl+O"));
	Aple->Append(ID_CLOSE, _("&Close\tCtrl+t"));
	//i didnt set menu bar bec i didnt see its neccasity but used an accerator table
	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'O', ID_OPEN);
	entries[1].Set(wxACCEL_CTRL, (int)'T', ID_CLOSE);
	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);
}

std::string mainFrame::getImagePath()
{
	/*char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	std::string fullPath(buffer);
	return fullPath.substr(0, fullPath.find_last_of("\\/"));		i commented it out bec though it works it doesnt compile on mac and other OS
	*/
	configPath = wxStandardPaths::Get().GetUserConfigDir().ToStdString() + "/App";
	std::filesystem::create_directories(configPath);
	return configPath+"/config.txt";
}

void mainFrame::checkImagePath()
{
	std::string config = getImagePath(); 
	if (!std::filesystem::exists(config)) return;
		std::ifstream inFile(config);
		std::string path;
		std::getline(inFile, path);
		pickUpFiles(path);
}

void mainFrame::registerStartup()	//everything will run on windows only
{
#ifdef __WXMSW__			//__WXMSW__ is a preprocessor directive that is defined when compiling for Windows using wxWidgets. It allows you to include Windows-specific code that will only be compiled and executed on Windows platforms.
	wxString exePath =wxStandardPaths::Get().GetExecutablePath();
	wxRegKey key(wxRegKey::HKCU, "Software\\Microsoft\\Windows\\CurrentVersion\\Run");
	key.SetValue("Aple", exePath);
#endif 
}

void mainFrame::openFile(wxCommandEvent& evt)
{
	wxDirDialog dlg(this, "Select a folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

	if (dlg.ShowModal() == wxID_CANCEL || dlg.GetPath().IsEmpty())return;
	pickUpFiles(dlg.GetPath().ToStdString());
	std::ofstream outFile(getImagePath());
	outFile << dlg.GetPath().ToStdString();

}

void mainFrame::closeWindow(wxCommandEvent& evt)
{
	int ans=wxMessageBox("window is closing app",
		"Confirmation",wxYES_NO | wxICON_INFORMATION);
	if (ans == wxYES) {
		std::string config = configPath + "config.txt";
		std::ifstream outFile(config);
		this->Close();
	}
}

void mainFrame::pickUpFiles(const std::string& folderPath)
{
	Imagepaths.clear();
	timer->Stop();
	if (!std::filesystem::exists(folderPath)) return;
	
	for (auto& entry : std::filesystem::directory_iterator(folderPath)) {
		std::string ext= entry.path().extension().string();
		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".jfif" || ext == ".webp" || ext == ".avif" || ext == ".heic")
			Imagepaths.push_back(entry.path().string());
		else
			wxLogMessage("skipping file: %s ", entry.path().string(),wxOK|wxICON_INFORMATION);
	}
	currentIndex = 0;
	if (Imagepaths.empty())return;
	std::shuffle(Imagepaths.begin(), Imagepaths.end(), engine);
	uploadImage(Imagepaths[0]);
	timer->Start(300000);
	removeTaskbar();
}

void mainFrame::fireTimer(wxTimerEvent& evt)
{
	
	if (Imagepaths.empty()) {
		timer->Stop();
		wxMessageBox("No image to be displayed", "Alert", wxOK | wxICON_INFORMATION);
		return;
	}
	currentIndex = (currentIndex + 1) % Imagepaths.size();			//creates a circular loop
	if (!std::filesystem::exists(Imagepaths[currentIndex])) {
		Imagepaths.erase(Imagepaths.begin()+currentIndex);
		if (Imagepaths.empty()) {
			timer->Stop();
			wxMessageBox("No files Found", "Warning", wxOK|wxICON_WARNING);
			return;
		}
		currentIndex = currentIndex % Imagepaths.size();
	}
	int nextIndex = (currentIndex + 1) % Imagepaths.size();
	if (nextIndex == 0)std::shuffle(Imagepaths.begin(), Imagepaths.end(), engine);
	uploadImage(Imagepaths[currentIndex]);
}

void mainFrame::OnPaint(wxPaintEvent& evt)
{
	wxPaintDC dc(glCanvas);
	glCanvas->SetCurrent(*glContext);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(-1, -1);
	glTexCoord2f(1, 0); glVertex2f(1, -1);
	glTexCoord2f(1, 1); glVertex2f(1, 1);
	glTexCoord2f(0, 1); glVertex2f(-1, 1);
	glEnd();

	glCanvas->SwapBuffers();
}




void mainFrame::onShow(wxShowEvent& evt)
{
	evt.Skip();
	checkImagePath();
}

void mainFrame::resize(wxSizeEvent& evt)
{
	evt.Skip();
	if (!glCanvas || !glContext)return;
	glCanvas->SetCurrent(*glContext);
	wxSize sz = glCanvas->GetClientSize();
	if (sz.x < 1 || sz.y<1)return;
	glViewport(0, 0, sz.x, sz.y);
	glCanvas->Refresh();
}

void mainFrame::swapp(wxMouseEvent& evt)
{
	if (Imagepaths.size() < 1)return;
	currentIndex = (currentIndex + 1)%Imagepaths.size();
	uploadImage(Imagepaths[currentIndex]);
	//wxMessageBox("Skiped image", "debug");
}

