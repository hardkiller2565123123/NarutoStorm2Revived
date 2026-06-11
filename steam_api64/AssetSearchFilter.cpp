#include "StdInc.h"
#include "AssetSearchFilter.h"

#include <regex>

namespace
{
    std::string Lower(std::string s)
    {
        std::transform(
            s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return s;
    }

    bool ContainsNoCase(const std::string& hay, const std::string& needle)
    {
        if (needle.empty())
            return true;

        return Lower(hay).find(Lower(needle)) != std::string::npos;
    }
}

namespace AssetSearchFilter
{
    bool Matches(const AssetBrowser::AssetEntry& asset, const Filter& filter)
    {
        if (filter.OverridesOnly && !(asset.HasModOverride || asset.IsLooseOverride))
            return false;

        if (filter.DumpedOnly && !asset.IsDumped && asset.DumpFullPath.empty())
            return false;

        if (filter.ExtractableOnly && asset.IsCpkEntry && asset.ArchiveEntryPath.empty())
            return false;

        if (filter.UnknownOnly && asset.Type != AssetBrowser::AssetType::Unknown)
            return false;

        if (!filter.Extension.empty() && !ContainsNoCase(asset.Extension, filter.Extension))
            return false;

        if (!filter.ArchiveContains.empty() && !ContainsNoCase(asset.ArchivePath, filter.ArchiveContains))
            return false;

        std::string hay =
            asset.Name + " " +
            asset.RelativePath + " " +
            asset.VirtualPath + " " +
            asset.FullPath + " " +
            asset.ArchivePath + " " +
            asset.ArchiveEntryPath + " " +
            asset.OverrideFullPath;

        for (const auto& dep : asset.Dependencies)
            hay += " " + dep;

        for (const auto& marker : asset.Markers)
            hay += " " + marker;

        if (filter.Search.empty())
            return true;

        if (filter.RegexMode)
        {
            try
            {
                std::regex r(filter.Search, std::regex::icase);
                return std::regex_search(hay, r);
            }
            catch (...)
            {
                return ContainsNoCase(hay, filter.Search);
            }
        }

        return ContainsNoCase(hay, filter.Search);
    }
}
