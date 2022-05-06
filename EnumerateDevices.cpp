#include <windows.h>
#include <dshow.h>

//#include <strmif.h>

#include <atlbase.h>


#pragma comment(lib, "strmiids")


void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = nullptr;
    }
    if (mt.pUnk != nullptr)
    {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = nullptr;
    }
}


// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != nullptr)
    {
        _FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}

HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}


void DisplayDeviceInformation(IEnumMoniker *pEnum)
{
    IMoniker *pMoniker = nullptr;

    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
    {
        IPropertyBag *pPropBag;
        HRESULT hr = pMoniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;
        }

        VARIANT var;
        VariantInit(&var);

        // Get description or friendly name.
        hr = pPropBag->Read(L"Description", &var, nullptr);
        if (FAILED(hr))
        {
            hr = pPropBag->Read(L"FriendlyName", &var, nullptr);
        }
        if (SUCCEEDED(hr))
        {
            printf("%S\n", var.bstrVal);
            VariantClear(&var);
        }

        //hr = pPropBag->Write(L"FriendlyName", &var);

        // WaveInID applies only to audio capture devices.
        hr = pPropBag->Read(L"WaveInID", &var, nullptr);
        if (SUCCEEDED(hr))
        {
            printf("WaveIn ID: %ld\n", var.lVal);
            VariantClear(&var);
        }

        hr = pPropBag->Read(L"DevicePath", &var, nullptr);
        if (SUCCEEDED(hr))
        {
            // The device path is not intended for display.
            printf("Device path: %S\n", var.bstrVal);
            VariantClear(&var);
        }

        CComPtr<IBaseFilter> ppDevice;
        bool deviceFound = false;

        //we get the filter
        if (SUCCEEDED(pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&ppDevice)))
        {
            //now the device is in use
            deviceFound = true;

            //CComPtr<IPin> pPin;
            //if (SUCCEEDED(ppDevice->FindPin(L"Output0", &pPin)))
            //{
            //    int i = 0;
            //}

            CComPtr<IEnumPins> pEnumPins;
            if (SUCCEEDED((ppDevice->EnumPins(&pEnumPins))))
            {
                IPin *pPin = nullptr;
                while (pEnumPins->Next(1, &pPin, nullptr) == S_OK)
                {
                    //PIN_DIRECTION direction;
                    PIN_INFO info;
                    if (SUCCEEDED(pPin->QueryPinInfo(&info))
                        && info.dir == PINDIR_OUTPUT)
                    {
                        CComPtr<IEnumMediaTypes> pEnum;
                        AM_MEDIA_TYPE *pmt = nullptr;
                        //BOOL bFound = FALSE;

                        if (SUCCEEDED(pPin->EnumMediaTypes(&pEnum)))
                        {
                            while (pEnum->Next(1, &pmt, nullptr) == S_OK)
                            {
                                if ((pmt->formattype == FORMAT_VideoInfo) &&
                                    (pmt->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
                                    (pmt->pbFormat != nullptr))
                                {
                                    auto videoInfoHeader = (VIDEOINFOHEADER*)pmt->pbFormat;
                                    printf("  Width: %ld; height: %ld\n",
                                        videoInfoHeader->bmiHeader.biWidth,  // Supported width
                                        videoInfoHeader->bmiHeader.biHeight); // Supported height
                                }

                                _DeleteMediaType(pmt);
                            }
                        }
                    }
                    if (pPin) {
                        pPin->Release();
                    }
                }
            }
        }

        pPropBag->Release();
        pMoniker->Release();
    }
}

int main()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        IEnumMoniker *pEnum;

        hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr))
        {
            DisplayDeviceInformation(pEnum);
            pEnum->Release();
        }
        /*
        hr = EnumerateDevices(CLSID_AudioInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr))
        {
            DisplayDeviceInformation(pEnum);
            pEnum->Release();
        }
        */
        CoUninitialize();
    }
}
