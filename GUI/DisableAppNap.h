// -*- C++ -*-

class DisableAppNap
{
    struct Impl;
public:
    DisableAppNap();

    ~DisableAppNap();

    void begin();
    void end();

private:
    Impl* impl_;
};
