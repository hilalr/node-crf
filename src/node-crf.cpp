//#include <node.h>

#include "node-crf.hpp"

#include <string>
#include <cstring>
#include <stdlib.h>

using v8::Array;
using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::Exception;
using v8::Handle;

//using namespace std;

Persistent<Function> CRF::constructor;

void CRF::Init(Local<Object> exports) {
    Isolate* isolate = Isolate::GetCurrent();
    
    NODE_SET_METHOD(exports, "learn", learn);
    
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "CRF"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    
    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "classify", classify);
    
    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "CRF"),
                 tpl->GetFunction());
}

CRF::CRF(){};

//Utility
char *CRF::get(Local<Value> value, const char *fallback = "") {
    if (value->IsString()) {
        v8::String::Utf8Value string(value->ToString());
        char *str = (char *) malloc(string.length() + 1);
        std::strcpy(str, *string);
        return str;
    }
    char *str = (char *) malloc(std::strlen(fallback) + 1);
    std::strcpy(str, fallback);
    return str;
}
//###################################################################

void CRF::New(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    if (args.IsConstructCall()) {
        // Invoked as constructor: `new CRF(...)`
        
        CRF* obj = new CRF();
        
        CRFPP::Tagger* tag = CRFPP::createTagger(get(args[0]));
        if(!tag){
            
            isolate->ThrowException(Exception::TypeError(
                                                       String::NewFromUtf8(isolate, (const char *) CRFPP::getTaggerError())));
            return;
            
        }
        
        v8::Local<v8::External> handle = v8::External::New(isolate, tag);
        v8::Persistent<v8::External, v8::CopyablePersistentTraits<v8::External> > tagger(isolate, handle);
        
        obj -> tagger = tagger;
        
        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
    } else {
        const int argc = 1;
        Local<Value> argv[argc] = { args[0] };
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
}

//Utility
static char * getUTF8String(Local<Value> value, const char *fallback = "") {
    if (value->IsString()) {
        v8::String::Utf8Value string(value->ToString());
        char *str = (char *) malloc(string.length() + 1);
        std::strcpy(str, *string);
        return str;
    }
    char *str = (char *) malloc(std::strlen(fallback) + 1);
    std::strcpy(str, fallback);
    return str;
}

void CRF::learn(const FunctionCallbackInfo<Value>& args){
    Isolate*        isolate         = NULL; 
    Local<Value>    final_result;
    int             result          = 0;
    char *          template_path   = NULL;
    char *          model_path      = NULL;
    char *          output_path     = NULL;
    
    
    isolate = args.GetIsolate();
    
    // Get template
    template_path = getUTF8String(args[0]);
    
    // Get training model path
    model_path = getUTF8String(args[1]);
    
    // Get output path
    output_path = getUTF8String(args[2]);
    
    // Prepare arguments
    char *argv[4];
    argv[0] = "crf_learn";
    argv[1] = template_path;
    argv[2] = model_path;
    argv[3] = output_path;
    
    // Learn
    result = crfpp_learn(4,argv);
    
    // Set result in v8 object
    final_result = v8::Integer::New(isolate, result);
    
    // Return
    args.GetReturnValue().Set(final_result);
}


void CRF::classify(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = args.GetIsolate();
    
    CRF* obj = (CRF *) ObjectWrap::Unwrap<CRF>(args.Holder());

    v8::Persistent<v8::External, v8::CopyablePersistentTraits<v8::External> > tagger = obj->tagger;
    v8::Local<v8::External> handle = v8::Local<v8::External>::New(isolate, tagger);
    CRFPP::Tagger *a = (CRFPP::Tagger *) handle->Value();
    
    fprintf(stderr, "address of tagger is %p\n",a);
    
    if (a==NULL) {
        fprintf(stderr,"No tagger. Exiting...\n");
        return;
    }
    
    a->clear();
    
    Local<Array> arr = Local<Array>::Cast(args[0]);
    
    int size = arr->Length();
    for(int i=0;i<size;i++){
        Local<Value> element = arr->Get(i);
        char* c = get(element);
        a->add(c);
        // each iteration, free memory
        free(c);
        c = NULL;
    }
    
    bool isParsed = a->parse();
    if ( isParsed == false) {
        fprintf(stderr,"Failed to parse.\n");
    }
    
    Local<Array> solutions = Array::New(isolate, a->nbest());
    
    unsigned int count = 0;
    while (count < a->nbest())
    {
        Local<Array> s = Array::New(isolate, a->ysize());
        for (size_t i = 0; i < a->size(); ++i)
        {
            s -> Set(i,Local<Value>(String::NewFromUtf8(isolate, a->y2(i))));
        }
        solutions -> Set(count, s);
        a->next();
        count++;
    }
    
    args.GetReturnValue().Set(solutions);
};

void RegisterModule(Handle<Object> target) {
    CRF::Init(target);
}

NODE_MODULE(nodecrf, RegisterModule);
