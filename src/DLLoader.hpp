#ifndef ARCADE_DLLOADER_HPP_
#define ARCADE_DLLOADER_HPP_

#include <dlfcn.h>
#include <stdexcept>
#include <string>

template <typename T>
class DLLoader {
public:
    DLLoader(const std::string &libPath)
        : _handle(nullptr), _libPath(libPath)
    {
        _handle = dlopen(libPath.c_str(), RTLD_LAZY);
        if (!_handle)
            throw std::runtime_error(std::string(dlerror()));
    }

    ~DLLoader()
    {
        if (_handle)
            dlclose(_handle);
    }

    DLLoader(const DLLoader &) = delete;
    DLLoader &operator=(const DLLoader &) = delete;

    T *getInstance(const std::string &entryPoint)
    {
        using creator_t = T *(*)();
        dlerror();
        creator_t creator = reinterpret_cast<creator_t>(
            dlsym(_handle, entryPoint.c_str()));
        const char *err = dlerror();
        if (err)
            throw std::runtime_error(std::string(err));
        return creator();
    }

    bool hasSymbol(const std::string &name) const
    {
        dlerror();
        (void)dlsym(_handle, name.c_str());
        const char *err = dlerror();
        return err == nullptr;
    }

    template <typename SymT>
    SymT getSymbol(const std::string &name) const
    {
        dlerror();
        SymT sym = reinterpret_cast<SymT>(dlsym(_handle, name.c_str()));
        const char *err = dlerror();
        if (err)
            throw std::runtime_error(std::string(err));
        return sym;
    }

    const std::string &getPath() const { return _libPath; }

private:
    void *_handle;
    std::string _libPath;
};

#endif
