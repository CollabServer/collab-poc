#include "MVKWrapper.h"

#include <iostream>
#include <sstream>
#include <regex>

#include "UUIDGenerator.h"


// -----------------------------------------------------------------------------
// Static util methods
// -----------------------------------------------------------------------------

static void MVKDebugDump(const char *text, FILE *stream, unsigned char *ptr,
                         size_t size, char nohex);
static int MVKDebugTrace(CURL *handle, curl_infotype type, char *data,
                         size_t size, void *userp);
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void* userp);

static void curlError(CURLcode code) {
    if(code != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
    }
}


// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------

MVKWrapper::MVKWrapper(bool debugMode) {
    _debugMode = debugMode;
    _uuid = generateNewUUID();
}


// -----------------------------------------------------------------------------
// User interface
// -----------------------------------------------------------------------------

int MVKWrapper::connect(const std::string& ip, const int port,
                         const std::string& username, const std::string& pswd) {
    // Init CURL
    curl_global_init(CURL_GLOBAL_ALL);
    _curl = curl_easy_init();
    std::stringstream addr;
    addr << "https://" << ip << ":" << port; // Format: "http://127.0.0.1:8001"
    curl_easy_setopt(_curl, CURLOPT_URL, addr.str().c_str());
    _dbAnswer = "";

    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_dbAnswer);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    _sendRequest = "&op=set_input&taskname=" + _uuid;
    _recvRequest = "op=get_output&taskname=" + _uuid;

    // This part usefull in case of we need a really big debug mode
    /*
    if(DebugMode) {
        MVKDebugData config;
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, MVKDebugTrace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
    */


    // Connect MVK
    std::string request;

    request = "op=set_input&value=\"" + _uuid + "\"&taskname=task_manager";
    send(request.c_str());
    if(!receive()) { return -1; }

    request = "value=\"" + username + "\"" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }

    std::regex newUser("^[A-Za-z \"]+new user");
    if(std::regex_search(_dbAnswer, newUser)) {
        request = "value=\"" + pswd + "\"" + _sendRequest;
        send(request.c_str());
        if(!receive()) { return -1; }
    }

    request = "value=\"" + pswd + "\"" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(!receive()) { return -1; }

    if(!_debugMode) {
        request = "value=\"quiet\"" + _sendRequest;
        send(request.c_str());
    }

    return 0;
}

int MVKWrapper::disconnect() {
    curl_easy_cleanup(_curl);
    curl_global_cleanup();
    return 0;
}


// -----------------------------------------------------------------------------
// Network
// -----------------------------------------------------------------------------

void MVKWrapper::send(const char* data) {
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, -1L);

    CURLcode curlAnswer = curl_easy_perform(_curl);
    if(_debugMode) {
        std::clog << "Sending: " << data << "\n";
        std::clog << "Answer: " << _dbAnswer << std::endl;
    }
    curlError(curlAnswer);
}

int MVKWrapper::receive() {
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, _recvRequest.c_str());
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, -1L);

    CURLcode curlAnswer = curl_easy_perform(_curl);
    if(_debugMode) {
        std::clog << "Sending: " << _recvRequest << "\n";
        std::clog << "Answer:" << _dbAnswer << std::endl;
    }
    curlError(curlAnswer);
    std::regex unknownCommand("^\"[Uu]nknown command");
    std::regex success("^\"Success");
    if(std::regex_search(_dbAnswer, unknownCommand)) {
        return 0;
    }
    return 0;

}


// -----------------------------------------------------------------------------
// Models management
// -----------------------------------------------------------------------------

int MVKWrapper::modelList(const std::string& path) {
    std::string request = "data=[\"model_list\",\"" + path + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    return 0;
}

int MVKWrapper::modelAdd(const std::string& name,
                         const std::string& mmodel,
                         const std::string& syntax) {
    std::string request;
    request = "data=[\"model_add\",\"" + mmodel + "\",\"" + name +
              "\",\"" + syntax + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }

    return 0;
}

int MVKWrapper::modelDelete(const std::string& name) {
    std::string request;
    request = "data=[\"model_delete\",\"" + name + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
    }
    return 0;
}

int MVKWrapper::modelVerify(const std::string& model, const std::string& mmodel) {
    std::string request;
    request = "data=[\"verify\",\"" + model + "\",\"" + mmodel + "\"]"
              + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }
    return 0;
}

int MVKWrapper::modelModify(const std::string& model,
                            const std::string& mmodel) {
    std::string request;
    request = "data=[\"model_modify\",\"" + model + "\",\"" + mmodel + "\"]"
              + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }
    return 0;
}

int MVKWrapper::modelExit() {
    std::string request = "value=\"exit\"" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }

    return 0;
}


// -----------------------------------------------------------------------------
// Model
// -----------------------------------------------------------------------------

int MVKWrapper::elementList() {
    std::string request = "value=\"list_full\"" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    return 0;
}

int MVKWrapper::elementListJSON() {
    std::string request = "value=\"JSON\"" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    return 0;
}

int MVKWrapper::elementCreate(const std::string& type, const std::string& name) {
    std::string request;
    request = "data=[\"instantiate_node\",\"" + type + "\",\"" + name + "\"]"
              + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }
    return 0;
}
int MVKWrapper::elementDelete(const std::string& name) {
    std::string request = "data=[\"delete\",\"" + name + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
    }
    return 0;
}

int MVKWrapper::edgeCreate(const std::string& type, const std::string& name,
                            const std::string& from, const std::string& to) {
    std::string request;
    request = "data=[\"instantiate_edge\",\"" + type + "\",\"" + name
              + "\",\"" + from + "\",\"" + to + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }
    return 0;
}

int MVKWrapper::attributeSet(const std::string& element,
                              const std::string& attrType,
                              const std::string& attrValue) {
    std::string request;
    request = "data=[\"attr_add\",\"" + element + "\",\"" + attrType
              + "\",\"" + attrValue + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }

    return 0;
}

int MVKWrapper::attributeDelete(const std::string& element,
                                 const std::string& attrType) {
    std::string request;
    request = "data=[\"attr_delete\",\"" + element + "\",\"" + attrType + "\"]"
              + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }

    return 0;
}

int MVKWrapper::attributeDefine(const std::string& element,
                                 const std::string& attrType,
                                 const std::string& attrName) {
    std::string request;
    request = "data=[\"define_attribute\",\"" + element + "\",\""
              + attrName + "\",\"" + attrType + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }
    return 0;
}

int MVKWrapper::attributeAddModifyCode(const std::string& element,
                                        const std::string& attrType,
                                        const std::string& attrValue) {
    std::string request;
    request = "data=[\"attr_add_code\",\"" + element + "\",\"" + attrType
              + "\",\"" + attrValue + "\"]" + _sendRequest;
    send(request.c_str());
    if(!receive()) { return -1; }
    if(_debugMode) {
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
        if(!receive()) { return -1; }
    }

    return 0;
}


// -----------------------------------------------------------------------------
// Getters / Setters
// -----------------------------------------------------------------------------

const std::string MVKWrapper::getCleanDatabaseAnswer() const {
    if(_dbAnswer.length() > 11) {
        return _dbAnswer.substr(10, _dbAnswer.length() - 11);
    }
    return "";
}


// -----------------------------------------------------------------------------
// Static Debug
// -----------------------------------------------------------------------------

struct MVKDebugData {
    char trace_ascii; /* 1 or 0 */
};

static void MVKDebugDump(const char *text, FILE *stream, unsigned char *ptr,
                         size_t size, char nohex) {
    size_t i;
    size_t c;

    unsigned int width = 0x10;

    if(nohex)
        /* without the hex output, we can fit more on screen */
        width = 0x40;

    fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n", text, size, size);

    for (i = 0; i < size; i += width) {
        fprintf(stream, "%4.4lx: ", i);

        if(!nohex) {
            /* hex not disabled, show it */
            for (c = 0; c < width; c++) {
                if(i + c < size)
                    fprintf(stream, "%02x ", ptr[i + c]);
                else
                    fputs("   ", stream);
            }
        }

        for (c = 0; (c < width) && (i + c < size); c++) {
            /* check for 0D0A; if found, skip past and start a new line of output */
            if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
                    ptr[i + c + 1] == 0x0A) {
                i += (c + 2 - width);
                break;
            }
            fprintf(stream, "%c",
                    (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c]
                                                                : '.');
            /* check again for 0D0A, to avoid an extra \n if it's at width */
            if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
                ptr[i + c + 2] == 0x0A) {
                i += (c + 3 - width);
                break;
            }
        }
        fputc('\n', stream);
    }
    fflush(stream);
}

static int MVKDebugTrace(CURL *handle, curl_infotype type, char *data,
                              size_t size, void *userp) {
    struct MVKDebugData *config = (struct MVKDebugData *) userp;
    const char *text;
    (void) handle; /* prevent compiler warning */

    switch (type) {
        case CURLINFO_TEXT:
            fprintf(stderr, "== Info: %s", data);
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;

        default: /* In case a new one is introduced to shock us */
            return 0;
    }

    MVKDebugDump(text, stderr, (unsigned char*) data, size, config->trace_ascii);
    return 0;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*) userp)->clear();
    ((std::string*) userp)->append((char*) contents, size* nmemb);
    return size * nmemb;
}
