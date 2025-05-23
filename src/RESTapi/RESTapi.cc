#include "RESTapi.h"

#include "httplib.h"


void RESTapi::run(){
    this->p_t = std::make_shared<std::thread>(std::bind(&RESTapi::worker, this));
    qInfo("API worker started!");
}


void RESTapi::worker(){

    httplib::Server svr;

    // GET: http://localhost:8080/PLANUPDATE?fp=<filepath>
    svr.Get("/PLANUPDATE", [this](const httplib::Request& req, httplib::Response& res) {
        try{

            std::string fp = req.get_param_value("fp");
            this->_currentFilePath = QString(fp.c_str());
            emit this->newMissionPlan();
            //TODO: achtung, das kann probleme verursachen, für landeplan und andere routen, vllt unterschiedliche routen oder weitere parameter einfügen.
            this->_landingPlanRequested = false;
            std::cout << "[API]: **** new missionpath: " << fp << " ****" << std::endl;

            this->_landButtonIcon = LANDBUTTONICON_NORMAL;
            emit this->landButtonIconChanged();
            res.set_content("OK", "text/plain");
        }
    catch(...){
            res.set_content("Error", "text/plain");
            std::cout << "[API][!] Fehlerhafte Anfrage :/" << std::endl;
        }
    });

    //GET: http://localhost:8080/LANDPLANSTATUS
    svr.Get("/LANDPLANSTATUS", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(this->_landingPlanRequested?"PLAN_REQUESTED":"NONE", "text/plain");
    });

    //GET: http://localhost:8080/MARKER?coords=<lat1,lon1,lat2,lon2....>
    svr.Get("/MARKER", [this](const httplib::Request& req, httplib::Response& res) {


        try{
            std::string param = req.get_param_value("coords");

            QVariantList coords;

            std::stringstream ss(param);
            std::string param_str;
            while(std::getline(ss, param_str,',')){
                coords << std::stod(param_str);
            }
            emit this->newMarkers(coords);
            std::cout << "new Markers: " << param << std::endl;

            res.set_content("OK", "text/plain");
        }
        catch(...){
            res.set_content("Error", "text/plain");
            std::cout << "[API][!] Fehlerhafte Anfrage :/" << std::endl;
        }
    });

    svr.listen("0.0.0.0", 8080);
}

QString RESTapi::message() const
{
    return _msg;
}

QString RESTapi::currentFilePath() const{
    return _currentFilePath;
}

QString RESTapi::landButtonIcon() const
{
    return _landButtonIcon;
}

void RESTapi::landButtonPressed()
{
    this->_landButtonIcon = LANDBUTTONICON_WAIT;
    this->_landingPlanRequested = true;
    emit this->landButtonIconChanged();
}
