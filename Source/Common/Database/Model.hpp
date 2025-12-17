#pragma once

#include <string>
#include <chrono>
#include <optional>

#include "Types.hpp"

#include "ResultDAO.hpp"
#include "OperatorDAO.hpp"
#include "CommandDAO.hpp"
#include "LogDAO.hpp"
#include "SessionDAO.hpp"
#include "VictimDAO.hpp"
#include "VictimTemplateDAO.hpp"

//--------------------------------
//
//--------------------------------
using User = OperatorDAO;

using VictimTemplate = VictimTemplateDAO;

using Victim = VictimDAO;

using Command = CommandDAO;

using Result = ResultDAO;

using Session = SessionDAO;

using Log = LogDAO;