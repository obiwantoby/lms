/*
 * Copyright (C) 2013 Emeric Poupon
 *
 * This file is part of LMS.
 *
 * LMS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LMS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AvFormatParser.hpp"

#include <algorithm>
#include <iostream>

#include "av/IAudioFile.hpp"
#include "utils/Logger.hpp"
#include "utils/String.hpp"
#include "Utils.hpp"

namespace MetaData
{

template <typename T>
std::optional<T>
findFirstValueOfAs(const Av::IAudioFile::MetadataMap& metadataMap, std::initializer_list<std::string> tags)
{
	auto it = std::find_first_of(std::cbegin(metadataMap), std::cend(metadataMap), std::cbegin(tags), std::cend(tags), [](const auto& it, const auto& str) { return it.first == str; });
	if (it == std::cend(metadataMap))
		return std::nullopt;

	return StringUtils::readAs<T>(StringUtils::stringTrim(it->second));
}

template <>
std::optional<std::vector<UUID>>
findFirstValueOfAs(const Av::IAudioFile::MetadataMap& metadataMap, std::initializer_list<std::string> tags)
{
	std::optional<std::string> str {findFirstValueOfAs<std::string>(metadataMap, tags)};
	if (!str)
		return std::nullopt;

	const std::vector<std::string_view> strUuids {StringUtils::splitString(*str, "/")};
	std::vector<UUID> res;

	for (std::string_view strUuid : strUuids)
	{
		std::optional<UUID> uuid {UUID::fromString(strUuid)};
		if (!uuid)
			return std::nullopt;

		res.push_back(std::move(*uuid));
	}

	return res;
}


static
std::optional<Album>
getAlbum(const Av::IAudioFile::MetadataMap& metadataMap)
{
	std::optional<Album> res;

	auto album {findFirstValueOfAs<std::string>(metadataMap, {"ALBUM"})};
	if (!album)
		return res;

	auto albumMBID {findFirstValueOfAs<UUID>(metadataMap, {"MUSICBRAINZ ALBUM ID", "MUSICBRAINZ_ALBUMID", "MUSICBRAINZ/ALBUM ID"})};

	return Album{*album, albumMBID};
}

static
std::vector<Artist>
getAlbumArtists(const Av::IAudioFile::MetadataMap& metadataMap)
{
	std::vector<Artist> res;

	auto name {findFirstValueOfAs<std::string>(metadataMap, {"ALBUM_ARTIST"})};
	if (!name)
		return res;

	auto mbid {findFirstValueOfAs<UUID>(metadataMap, {"MUSICBRAINZ ALBUM ARTIST ID", "MUSICBRAINZ/ALBUM ARTIST ID"})};

	return {Artist {*name, std::nullopt, mbid} };
}

static
std::vector<Artist>
getArtists(const Av::IAudioFile::MetadataMap& metadataMap)
{
	std::vector<Artist> artists;

	std::vector<std::string_view> artistNames;
	if (metadataMap.find("ARTISTS") != metadataMap.end())
	{
		artistNames = StringUtils::splitString(metadataMap.find("ARTISTS")->second, "/;");
	}
	else if (metadataMap.find("ARTIST") != metadataMap.end())
	{
		artistNames = {metadataMap.find("ARTIST")->second};
	}

	auto artistMBIDs {findFirstValueOfAs<std::vector<UUID>>(metadataMap, {"MUSICBRAINZ ARTIST ID", "MUSICBRAINZ_ARTISTID", "MUSICBRAINZ/ARTIST ID"})};

	for (std::size_t i {}; i < artistNames.size(); ++i)
	{
		if (artistMBIDs && artistNames.size() == artistMBIDs->size())
			artists.emplace_back(Artist {artistNames[i], std::nullopt, (*artistMBIDs)[i]});
		else
			artists.emplace_back(Artist {artistNames[i], std::nullopt, {}});
	}

	return artists;
}

std::optional<Track>
AvFormatParser::parse(const std::filesystem::path& p, bool debug)
{
	Track track;

	try
	{
		const auto mediaFile {Av::parseAudioFile(p)};

		// Stream info
		{
			std::vector<AudioStream> audioStreams;

			for (auto stream : mediaFile->getStreamInfo())
			{
				MetaData::AudioStream audioStream {static_cast<unsigned>(stream.bitrate)};
				track.audioStreams.emplace_back(audioStream);
			}
		}

		track.duration = mediaFile->getDuration();
		track.hasCover = mediaFile->hasAttachedPictures();

		MetaData::Clusters clusters;

		const Av::IAudioFile::MetadataMap metadataMap {mediaFile->getMetaData()};

		for (const auto& [tag, value] : metadataMap)
		{
			if (debug)
				std::cout << "TAG = " << tag << ", VAL = " << value << std::endl;

			if (tag == "TITLE")
				track.title = value;
			else if (tag == "TRACK")
			{
				// Expecting 'Number/Total'
				const std::vector<std::string_view> strings {StringUtils::splitString(value, "/") };
				if (strings.size() > 0)
				{
					track.trackNumber = StringUtils::readAs<std::size_t>(strings[0]);

					if (strings.size() > 1)
						track.totalTrack = StringUtils::readAs<std::size_t>(strings[1]);
				}
			}
			else if (tag == "DISC")
			{
				// Expecting 'Number/Total'
				const std::vector<std::string_view> strings {StringUtils::splitString(value, "/")};
				if (strings.size() > 0)
				{
					track.discNumber = StringUtils::readAs<std::size_t>(strings[0]);

					if (strings.size() > 1)
						track.totalDisc = StringUtils::readAs<std::size_t>(strings[1]);
				}
			}
			else if (tag == "DATE"
					|| tag == "YEAR"
					|| tag == "WM/Year")
			{
				track.date = Utils::parseDate(value);
			}
			else if (tag == "TDOR"	// Original release time (ID3v2 2.4)
					|| tag == "TORY")	// Original release year
			{
				track.originalDate = Utils::parseDate(value);
			}
			else if (tag == "ACOUSTID ID")
			{
				track.acoustID = UUID::fromString(value);
			}
			else if (tag == "MUSICBRAINZ RELEASE TRACK ID"
					|| tag == "MUSICBRAINZ_RELEASETRACKID")
			{
				track.trackMBID = UUID::fromString(value);
			}
			else if (tag == "MUSICBRAINZ_TRACKID"
					|| tag == "MUSICBRAINZ/TRACK ID")
			{
				track.recordingMBID = UUID::fromString(value);
			}
			else if (tag == "TSST"
					|| tag == "DISCSUBTITLE"
					|| tag == "SETSUBTITLE")
			{
				track.discSubtitle = value;
			}
			else if (_clusterTypeNames.find(tag) != _clusterTypeNames.end())
			{
				const std::vector<std::string_view> clusterNames {StringUtils::splitString(value, "/,;")};

				if (!clusterNames.empty())
				{
					std::set<std::string> values;
					std::transform(std::cbegin(clusterNames), std::cend(clusterNames),
							std::inserter(values, std::begin(values)),
							[](std::string_view clusterName) { return std::string {clusterName}; });
					track.clusters[tag] = std::move(values);
				}
			}
		}

		track.artists = getArtists(metadataMap);
		track.album = getAlbum(metadataMap);
		track.albumArtists = getAlbumArtists(metadataMap);
	}
	catch(Av::Exception& e)
	{
		return std::nullopt;
	}

	return track;
}

} // namespace MetaData

