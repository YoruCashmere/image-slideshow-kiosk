
#include "App.h"
#include <wx/msw/private.h>
wxIMPLEMENT_APP(App);
bool App::OnInit()
{
    wxInitAllImageHandlers();
    mainFrame* window = new mainFrame("Aple");
    window->SetClientSize(350,250);
    window->Show();
    return true;
}
