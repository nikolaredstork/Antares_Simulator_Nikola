/*
** Copyright 2007-2024, RTE (https://www.rte-france.com)
** See AUTHORS.txt
** SPDX-License-Identifier: MPL-2.0
** This file is part of Antares-Simulator,
** Adequacy and Performance assessment for interconnected energy networks.
**
** Antares_Simulator is free software: you can redistribute it and/or modify
** it under the terms of the Mozilla Public Licence 2.0 as published by
** the Mozilla Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** Antares_Simulator is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** Mozilla Public Licence 2.0 for more details.
**
** You should have received a copy of the Mozilla Public Licence 2.0
** along with Antares_Simulator. If not, see <https://opensource.org/license/mpl-2-0/>.
*/

#include <algorithm>

#include <yuni/yuni.h>
#include <yuni/io/file.h>

#include <antares/exception/LoadingError.hpp>
#include <antares/inifile/inifile.h>
#include <antares/logs/logs.h>
#include <antares/study/parts/hydro/hydroreservoirlevels.h>
#include "antares/study/study.h"

namespace fs = std::filesystem;

namespace Antares::Data
{

static std::unique_ptr<ReservoirLevelsLoader> createReservoirLevelLoader(
  Parameters::Compatibility::HydroRuleCurves hydroRuleCurves,
  const std::filesystem::path& filePath,
  const std::string& areaID,
  Matrix<double>& standardReservoirLevelMatrix,
  TimeSeries& max,
  TimeSeries& avg,
  TimeSeries& min)
{
    switch (hydroRuleCurves)
    {
    case Parameters::Compatibility::HydroRuleCurves::Single:
    {
        return std::make_unique<StandardReservoirLevelsLoader>(filePath,
                                                               areaID,
                                                               standardReservoirLevelMatrix,
                                                               max,
                                                               avg,
                                                               min);
    }
    case Parameters::Compatibility::HydroRuleCurves::Scenarized:
    {
        return std::make_unique<ScenarizedReservoirLevelLoader>(filePath, areaID, max, avg, min);
    }
    default:
        throw std::invalid_argument("Value not supported for hydro rule curves compatibility");
    }
}

ReservoirLevels::ReservoirLevels(TimeSeriesNumbers& timeseriesNumbers):
    max(timeseriesNumbers),
    min(timeseriesNumbers),
    avg(timeseriesNumbers),
    timeseriesNumbers(timeseriesNumbers)
{
    timeseriesNumbers.registerSeries(&max, "max-reservoir-level");
    timeseriesNumbers.registerSeries(&min, "min-reservoir-level");
    timeseriesNumbers.registerSeries(&avg, "avg-reservoir-level");

    max.reset(1L, DAYS_PER_YEAR);
    max.fill(1.0);
    avg.reset(1L, DAYS_PER_YEAR);
    avg.fill(0.5);
    min.reset(1L, DAYS_PER_YEAR);
    standardReservoirLevelMatrix.reset(3L, DAYS_PER_YEAR, true);
    standardReservoirLevelMatrix.fillColumn(ReservoirLevels::maximum, 1.);
    standardReservoirLevelMatrix.fillColumn(ReservoirLevels::average, 0.5);
}

bool ReservoirLevels::forceReload(bool reload) const
{
    bool ret = true;
    ret = max.forceReload(reload) && ret;
    ret = min.forceReload(reload) && ret;
    ret = avg.forceReload(reload) && ret;
    ret = standardReservoirLevelMatrix.forceReload(reload) && ret;

    return ret;
}

void ReservoirLevels::markAsModified() const
{
    max.markAsModified();
    min.markAsModified();
    avg.markAsModified();
    standardReservoirLevelMatrix.markAsModified();
}

bool ReservoirLevels::loadReservoirLevels(
  const std::string& areaID,
  const std::filesystem::path& folder,
  bool usedBySolver,
  Parameters::Compatibility::HydroRuleCurves hydroRuleCurves)
{
    bool ret = true;

    if (!usedBySolver)
    {
        Matrix<>::BufferType fileContent;
        fs::path filePath = folder / "common" / "capacity"
                            / std::string("reservoir_" + areaID + ".txt");
        standardReservoirLevelMatrix.reset(3, DAYS_PER_YEAR, true);
        bool enabledModeIsChanged = false;

        if (JIT::enabled)
        {
            JIT::enabled = false;
            enabledModeIsChanged = true;
        }

        ret = standardReservoirLevelMatrix.loadFromCSVFile(filePath.string(),
                                                           3,
                                                           DAYS_PER_YEAR,
                                                           Matrix<>::optFixedSize,
                                                           &fileContent)
              && ret;

        if (enabledModeIsChanged)
        {
            JIT::enabled = true; // Back to the previous loading mode.
        }
    }
    else
    {
        auto loader = createReservoirLevelLoader(hydroRuleCurves,
                                                 folder,
                                                 areaID,
                                                 standardReservoirLevelMatrix,
                                                 max,
                                                 avg,
                                                 min);
        ret = loader->load() && ret;
    }
    return ret;
}

bool ReservoirLevels::saveToFolder(const std::string& areaID, const std::string& folder) const
{
    bool ret = true;
    std::string buffer;
    buffer = folder + "/" + "common" + "/" + "capacity" + "/" + "reservoir_" + areaID + ".txt";

    ret = standardReservoirLevelMatrix.saveToCSVFile(buffer, /*decimal*/ 3) && ret;

    return ret;
}

void ReservoirLevels::averageTimeSeries()
{
    max.averageTimeseries();
    min.averageTimeseries();
    avg.averageTimeseries();
}

bool ScenarizedReservoirLevelLoader::load()
{
    const std::filesystem::path path = _baseFolder / "series" / _areaID;
    Matrix<>::BufferType fileContent;

    bool ret = true;

    fs::path filePath = path / "maxDailyReservoirLevels.txt";
    ret = _max.timeSeries.loadFromCSVFile(filePath.string(), 1, DAYS_PER_YEAR, &fileContent) && ret;
    filePath = path / "minDailyReservoirLevels.txt";
    ret = _min.timeSeries.loadFromCSVFile(filePath.string(), 1, DAYS_PER_YEAR, &fileContent) && ret;
    filePath = path / "avgDailyReservoirLevels.txt";
    ret = _avg.timeSeries.loadFromCSVFile(filePath.string(), 1, DAYS_PER_YEAR, &fileContent) && ret;

    return ret;
}

bool StandardReservoirLevelsLoader::load()
{
    const std::filesystem::path filePath = _baseFolder / "common" / "capacity"
                                           / std::string("reservoir_" + _areaID + ".txt");
    Matrix<>::BufferType fileContent;
    bool ret = true;

    ret = _standardReservoirLevelMatrix.loadFromCSVFile(filePath.string(),
                                                        3,
                                                        DAYS_PER_YEAR,
                                                        Matrix<>::optFixedSize,
                                                        &fileContent)
          && ret;
    copyReservoirLevelsFromBuffer();

    return ret;
}

void StandardReservoirLevelsLoader::copyReservoirLevelsFromBuffer()
{
    _min.timeSeries.reset(1U, DAYS_PER_YEAR, true);
    _min.timeSeries.pasteToColumn(0, _standardReservoirLevelMatrix[ReservoirLevels::minimum]);
    _avg.timeSeries.reset(1U, DAYS_PER_YEAR, true);
    _avg.timeSeries.pasteToColumn(0, _standardReservoirLevelMatrix[ReservoirLevels::average]);
    _max.timeSeries.reset(1U, DAYS_PER_YEAR, true);
    _max.timeSeries.pasteToColumn(0, _standardReservoirLevelMatrix[ReservoirLevels::maximum]);
}

} // namespace Antares::Data
