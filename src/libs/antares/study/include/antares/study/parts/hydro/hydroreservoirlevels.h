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
#ifndef __ANTARES_LIBS_STUDY_PARTS_HYDRO_RESERVOIR_LEVELS_H__
#define __ANTARES_LIBS_STUDY_PARTS_HYDRO_RESERVOIR_LEVELS_H__

#include <antares/array/matrix.h>
#include <antares/series/series.h>
#include <antares/study/parameters.h>
#include <antares/study/version.h>

#include "../../fwd.h"

namespace Antares
{
namespace Data
{

/*!
** \brief Reservoir Levels Class
*/

class ReservoirLevels
{
public:
    enum
    {
        //! The minimum value
        minimum = 0,
        //! The average value
        average,
        //! The maximum value
        maximum,
    };

    //! \name Constructor
    //@{
    /*!
    ** \brief Default constructor
    */
    ReservoirLevels(TimeSeriesNumbers& timeseriesNumbers);

    bool loadReservoirLevels(const std::string& areaID,
                             const std::filesystem::path& folder,
                             bool usedBySolver,
                             Parameters::Compatibility::HydroRuleCurves hydroRuleCurves);

    bool forceReload(bool reload = false) const;

    void markAsModified() const;

    bool saveToFolder(const std::string& areaID, const std::string& folder) const;

    void averageTimeSeries();

    /*!
     ** \brief Maximum Reservoir Levels (%)
     **
     ** Merely a matrix of TimeSeriesCount * DAYS_PER_YEAR values
     */
    TimeSeries max;

    /*!
    ** \brief Minimum Reservoir Levels (%)
    **
    ** Merely a matrix of TimeSeriesCount * DAYS_PER_YEAR values
    */
    TimeSeries min;

    /*!
    ** \brief Average Reservoir Levels (%)
    **
    ** Merely a matrix of TimeSeriesCount * DAYS_PER_YEAR values
    */
    TimeSeries avg;

    /*!
    ** \brief This matrix is used for UI communication
    **
    ** Merely a matrix of 3 * DAYS_PER_YEAR values
    */

    // The buffer facilitates communication with the UI.
    // This functionality will no longer be required once the UI is fully deprecated.
    Matrix<double> standardReservoirLevelMatrix;

    TimeSeriesNumbers& timeseriesNumbers;
};

class ReservoirLevelsLoader
{
public:
    ReservoirLevelsLoader(

      const std::filesystem::path& baseFolder,
      const std::string& areaID,
      TimeSeries& max,
      TimeSeries& avg,
      TimeSeries& min):
        _baseFolder(baseFolder),
        _areaID(areaID),
        _max(max),
        _avg(avg),
        _min(min)

    {
    }

    virtual ~ReservoirLevelsLoader() = default;
    virtual bool load() = 0;

protected:
    const std::filesystem::path& _baseFolder;
    const std::string& _areaID;
    TimeSeries& _max;
    TimeSeries& _avg;
    TimeSeries& _min;
};

class StandardReservoirLevelsLoader: public ReservoirLevelsLoader
{
public:
    StandardReservoirLevelsLoader(const std::filesystem::path& baseFolder,
                                  const std::string& areaID,
                                  Matrix<double>& standardReservoirLevelMatrix,
                                  TimeSeries& max,
                                  TimeSeries& avg,
                                  TimeSeries& min):
        ReservoirLevelsLoader(baseFolder, areaID, max, avg, min),
        _standardReservoirLevelMatrix(standardReservoirLevelMatrix)

    {
    }

private:
    Matrix<double>& _standardReservoirLevelMatrix;
    bool load() override final;
    void copyReservoirLevelsFromBuffer();
};

class ScenarizedReservoirLevelLoader: public ReservoirLevelsLoader
{
public:
    ScenarizedReservoirLevelLoader(const std::filesystem::path& baseFolder,
                                   const std::string& areaID,
                                   TimeSeries& max,
                                   TimeSeries& avg,
                                   TimeSeries& min):
        ReservoirLevelsLoader(baseFolder, areaID, max, avg, min)
    {
    }

private:
    bool load() override final;
};
} // namespace Data
} // namespace Antares

#endif /* __ANTARES_LIBS_STUDY_PARTS_HYDRO_RESERVOIR_LEVELS_H__ */
