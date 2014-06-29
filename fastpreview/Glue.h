#pragma once

class CoGlue
{
public:
  CoGlue()
  {
    (void)CoInitialize(nullptr);
  }

  ~CoGlue()
  {
    CoUninitialize();
  }
};