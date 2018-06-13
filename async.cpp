#include <napi.h>

// this class is used to enqueue an async error callback.
// if we want to send an error to the client via the client's 
// provided callback 
class ErrorAsyncWorker : public Napi::AsyncWorker
{
public:
    ErrorAsyncWorker( const Napi::Function& callback, Napi::Error error )
    : Napi::AsyncWorker{ callback }
    , error{ error }
    {

    }

protected:
    void Execute() override
    {
        // Do nothing then OnOK we will send an error
    }


    void OnOK() override
    {
        Napi::Env env = Env();
        Callback().MakeCallback( Receiver().Value(), { error.Value(), env.Undefined() } );
    }


    void OnError( const Napi::Error& e ) override
    {
        Napi::Env env = Env();
        Callback().MakeCallback( Receiver().Value(), { e.Value(), env.Undefined() } );
    }

private:
    Napi::Error error;
};

// this class will be instantiated and added to the queue. when the event loop
// is ready, it will call Execute() on this class. after Execute() successfully
// completes, the loop will call OnOK(), or if there is an issue it will call
// OnError. the callback is stored in the parent class, AsyncWorker via the
// parent class constructor
class SumAsyncWorker : public Napi::AsyncWorker
{
public:
    SumAsyncWorker( const Napi::Function& callback, const double arg0, const double arg1 )
    : Napi::AsyncWorker{ callback }
    , myArg0{ arg0 }
    , myArg1{ arg1 }
    , mySum{ 0 }
    {

    }

protected:
    void Execute() override
    {
        // do the work here
        mySum = myArg0 + myArg1;
    }


    void OnOK() override
    {
        // return successful values here
        this->Callback().MakeCallback( this->Receiver().Value(), { this->Env().Null(), Napi::Number::New( this->Env(), mySum ) } );
    }


    void OnError( const Napi::Error& e ) override
    {
        // return error here
        this->Callback().MakeCallback( this->Receiver().Value(), { e.Value(), this->Env().Undefined() } );
    }

private:
    double myArg0;
    double myArg1;
    double mySum;
};


void SumAsyncCallback( const Napi::CallbackInfo& info )
{
    Napi::Env env = info.Env();

    // we need to check for the presence of a client callback function
    // and if we do not have one then we must raise a synchronous error
    if( info.Length() < 3 )
    {
        Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
        return;
    }

    if( !info[2].IsFunction() )
    {
        Napi::TypeError::New(env, "Invalid argument types").ThrowAsJavaScriptException();
        return;
    }


    // handle other cases by sending an error out through the client's
    // callback function. this is the preferred way to send an error
    Napi::Function cb = info[2].As<Napi::Function>();

    if ( info.Length() != 3 )
    {
        (new ErrorAsyncWorker(cb, Napi::TypeError::New(env, "Invalid argument count")))->Queue();
    }
    else if (!info[0].IsNumber() || !info[1].IsNumber())
    {
        (new ErrorAsyncWorker(cb, Napi::TypeError::New(env, "Invalid argument types")))->Queue();
    }
    else
    {
        double arg0 = info[0].As<Napi::Number>().DoubleValue();
        double arg1 = info[1].As<Napi::Number>().DoubleValue();

        // create a new instance of the async worker class using operator new
        // and hand that pointer off to the engine which will clean up the
        // resources when it is done with execution (by 'delete this'?)
        SumAsyncWorker* sumAsyncWorker = new SumAsyncWorker( cb, arg0, arg1 );
        sumAsyncWorker->Queue();
    }

    return;
}


Napi::Object Init( Napi::Env env, Napi::Object exports )
{
    auto name = Napi::String::New(env, "add");
    auto function = Napi::Function::New( env, SumAsyncCallback );
    exports.Set( name, function );
    return exports;
}


NODE_API_MODULE( example, Init );
