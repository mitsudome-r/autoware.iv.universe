/*
 * Copyright 2020 Tier IV, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <autoware_state_monitor/autoware_state_monitor_node.h>

#include <numeric>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <boost/bind.hpp>

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include <autoware_state_monitor/rosconsole_wrapper.h>

void AutowareStateMonitorNode::setupDiagnosticUpdater()
{
  updater_.setHardwareID("autoware_state_monitor");

  std::vector<std::string> module_names;
  private_nh_.param("module_names", module_names, {});

  // Topic
  for (const auto & module_name : module_names) {
    const auto diag_name = fmt::format("{}_topic_status", module_name);

    updater_.add(
      diag_name, boost::bind(&AutowareStateMonitorNode::checkTopicStatus, this, _1, module_name));
  }

  // TF
  updater_.add(
    "localization_tf_status",
    boost::bind(&AutowareStateMonitorNode::checkTfStatus, this, _1, "localization"));
}

void AutowareStateMonitorNode::checkTopicStatus(
  diagnostic_updater::DiagnosticStatusWrapper & stat, const std::string & module_name)
{
  int8_t level = diagnostic_msgs::DiagnosticStatus::OK;

  const auto & topic_stats = state_input_.topic_stats;
  const auto & tf_stats = state_input_.tf_stats;

  // OK
  for (const auto & topic_config : topic_stats.ok_list) {
    if (topic_config.module != module_name) {
      continue;
    }

    stat.add(fmt::format("{} status", topic_config.name), "OK");
  }

  // Check topic received
  for (const auto & topic_config : topic_stats.non_received_list) {
    if (topic_config.module != module_name) {
      continue;
    }

    stat.add(fmt::format("{} status", topic_config.name), "Not Received");

    level = diagnostic_msgs::DiagnosticStatus::ERROR;
  }

  // Check topic rate
  for (const auto & topic_config_pair : topic_stats.slow_rate_list) {
    const auto & topic_config = topic_config_pair.first;
    const auto & topic_rate = topic_config_pair.second;

    if (topic_config.module != module_name) {
      continue;
    }

    const auto & name = topic_config.name;
    stat.add(fmt::format("{} status", name), "Slow Rate");
    stat.addf(fmt::format("{} warn_rate", name), "%.2f [Hz]", topic_config.warn_rate);
    stat.addf(fmt::format("{} measured_rate", name), "%.2f [Hz]", topic_rate);

    level = diagnostic_msgs::DiagnosticStatus::WARN;
  }

  // Check topic timeout
  for (const auto & topic_config_pair : topic_stats.timeout_list) {
    const auto & topic_config = topic_config_pair.first;
    const auto & last_received_time = topic_config_pair.second;

    if (topic_config.module != module_name) {
      continue;
    }

    const auto & name = topic_config.name;
    stat.add(fmt::format("{} status", name), "Timeout");
    stat.addf(fmt::format("{} timeout", name), "%.2f [s]", topic_config.timeout);
    stat.addf(fmt::format("{} checked_time", name), "%.2f [s]", topic_stats.checked_time.toSec());
    stat.addf(fmt::format("{} last_received_time", name), "%.2f [s]", last_received_time.toSec());

    level = diagnostic_msgs::DiagnosticStatus::ERROR;
  }

  // Create message
  std::string msg;
  if (level == diagnostic_msgs::DiagnosticStatus::OK) {
    msg = "OK";
  } else if (level == diagnostic_msgs::DiagnosticStatus::WARN) {
    msg = "Warn";
  } else if (level == diagnostic_msgs::DiagnosticStatus::WARN) {
    msg = "Error";
  }

  stat.summary(level, msg);
}

void AutowareStateMonitorNode::checkTfStatus(
  diagnostic_updater::DiagnosticStatusWrapper & stat, const std::string & module_name)
{
  int8_t level = diagnostic_msgs::DiagnosticStatus::OK;

  const auto & topic_stats = state_input_.topic_stats;
  const auto & tf_stats = state_input_.tf_stats;

  // OK
  for (const auto & tf_config : tf_stats.ok_list) {
    if (tf_config.module != module_name) {
      continue;
    }

    const auto name = fmt::format("{}2{}", tf_config.from, tf_config.to);
    stat.add(fmt::format("{} status", name), "OK");
  }

  // Check tf timeout
  for (const auto & tf_config_pair : tf_stats.timeout_list) {
    const auto & tf_config = tf_config_pair.first;
    const auto & last_received_time = tf_config_pair.second;

    if (tf_config.module != module_name) {
      continue;
    }

    const auto name = fmt::format("{}2{}", tf_config.from, tf_config.to);
    stat.add(fmt::format("{} status", name), "Timeout");
    stat.addf(fmt::format("{} timeout", name), "%.2f [s]", tf_config.timeout);
    stat.addf(fmt::format("{} checked_time", name), "%.2f [s]", tf_stats.checked_time.toSec());
    stat.addf(fmt::format("{} last_received_time", name), "%.2f [s]", last_received_time.toSec());

    level = diagnostic_msgs::DiagnosticStatus::ERROR;
  }

  // Create message
  std::string msg;
  if (level == diagnostic_msgs::DiagnosticStatus::OK) {
    msg = "OK";
  } else if (level == diagnostic_msgs::DiagnosticStatus::WARN) {
    msg = "Warn";
  } else if (level == diagnostic_msgs::DiagnosticStatus::WARN) {
    msg = "Error";
  }

  stat.summary(level, msg);
}
