#include "file_system.h"

#include <fstream>
#include <iterator>

namespace
{
    class HostFileSystem : public IFileSystem
    {
    public:
        bool exists(const char* path) override
        {
            std::ifstream file(path);
            return file.good();
        }

        bool readFile(const char* path, std::string& content) override
        {
            std::ifstream inFile(path);
            if (! inFile)
            {
                return false;
            }

            content = std::string((std::istreambuf_iterator<char>(inFile)),
                                  std::istreambuf_iterator<char>());
            return true;
        }

        bool writeFile(const char* path, const std::string& content) override
        {
            std::ofstream outFile(path);
            if (! outFile)
            {
                return false;
            }

            outFile << content;
            return outFile.good();
        }
    };
} // namespace

IFileSystem* createFileSystem()
{
    static HostFileSystem instance;
    return &instance;
}
