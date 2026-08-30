#include "result_storage.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace p0::track {

std::string ResultStorage::format_time(double time_sec) {
  if (time_sec <= 0.0) return "--:--.---";
  int minutes = static_cast<int>(time_sec / 60.0);
  double seconds = time_sec - minutes * 60.0;
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << minutes << ":"
      << std::fixed << std::setprecision(3) << std::setw(6) << seconds;
  return oss.str();
}

std::string ResultStorage::current_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

std::string ResultStorage::generate_race_id(const std::string& track_id, int car_id) {
  return track_id + "_car" + std::to_string(car_id) + "_" + current_timestamp();
}

std::string ResultStorage::default_results_path() {
  return "data/results.json";
}

std::string escape_json_string(const std::string& str) {
  std::string out;
  out.reserve(str.size());
  for (char c : str) {
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:   out += c; break;
    }
  }
  return out;
}

bool ResultStorage::save_results(const std::string& filepath, const ResultsDatabase& db) {
  std::ofstream ofs(filepath);
  if (!ofs) return false;

  const auto& results = db.all_results();
  ofs << "{\n";
  ofs << "  \"version\": \"1.0\",\n";
  ofs << "  \"result_count\": " << results.size() << ",\n";
  ofs << "  \"results\": [\n";

  for (size_t i = 0; i < results.size(); ++i) {
    const auto& r = results[i];
    ofs << "    {\n";
    ofs << "      \"race_id\": \"" << escape_json_string(r.race_id) << "\",\n";
    ofs << "      \"track_id\": \"" << escape_json_string(r.track_id) << "\",\n";
    ofs << "      \"track_name\": \"" << escape_json_string(r.track_name) << "\",\n";
    ofs << "      \"car_id\": " << r.car_id << ",\n";
    ofs << "      \"driver_name\": \"" << escape_json_string(r.driver_name) << "\",\n";
    ofs << "      \"completed_laps\": " << r.completed_laps << ",\n";
    ofs << "      \"total_time\": " << r.total_time << ",\n";
    ofs << "      \"best_lap_time\": " << r.best_lap_time << ",\n";
    ofs << "      \"session_date\": \"" << escape_json_string(r.session_date) << "\",\n";
    ofs << "      \"finished\": " << (r.finished ? "true" : "false") << ",\n";
    ofs << "      \"dnf\": " << (r.dnf ? "true" : "false") << ",\n";
    ofs << "      \"dnf_reason\": \"" << escape_json_string(r.dnf_reason) << "\",\n";
    ofs << "      \"lap_times\": [\n";
    for (size_t j = 0; j < r.lap_times.size(); ++j) {
      const auto& lt = r.lap_times[j];
      ofs << "        {\"lap_number\": " << lt.lap_number
          << ", \"lap_time\": " << lt.lap_time
          << ", \"valid\": " << (lt.valid ? "true" : "false")
          << ", \"timestamp\": " << lt.timestamp << "}";
      if (j + 1 < r.lap_times.size()) ofs << ",";
      ofs << "\n";
    }
    ofs << "      ]\n";
    ofs << "    }";
    if (i + 1 < results.size()) ofs << ",";
    ofs << "\n";
  }

  ofs << "  ]\n";
  ofs << "}\n";
  return true;
}

bool ResultStorage::load_results(const std::string& filepath, ResultsDatabase& db) {
  std::ifstream ifs(filepath);
  if (!ifs) return false;

  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  db.clear();

  size_t pos = 0;
  auto find_key = [&](const std::string& key) -> std::string::size_type {
    std::string search = "\"" + key + "\"";
    return content.find(search, pos);
  };

  auto extract_string = [&](std::string::size_type start) -> std::string {
    start = content.find(':', start);
    if (start == std::string::npos) return "";
    ++start;
    while (start < content.size() && (content[start] == ' ' || content[start] == '\t')) ++start;
    if (start >= content.size() || content[start] != '"') return "";
    ++start;
    std::string out;
    while (start < content.size()) {
      char c = content[start++];
      if (c == '\\' && start < content.size()) {
        char next = content[start++];
        if (next == 'n') out += '\n';
        else if (next == 't') out += '\t';
        else if (next == '\\') out += '\\';
        else if (next == '"') out += '"';
        else { out += c; out += next; }
      } else if (c == '"') {
        break;
      } else {
        out += c;
      }
    }
    return out;
  };

  auto extract_number = [&](std::string::size_type start) -> double {
    start = content.find(':', start);
    if (start == std::string::npos) return 0.0;
    ++start;
    while (start < content.size() && (content[start] == ' ' || content[start] == '\t')) ++start;
    std::string num_str;
    while (start < content.size() &&
           (std::isdigit(content[start]) || content[start] == '.' || content[start] == '-' || content[start] == 'e' || content[start] == 'E')) {
      num_str += content[start++];
    }
    if (num_str.empty() || num_str == "true" || num_str == "false") return 0.0;
    return std::stod(num_str);
  };

  auto extract_bool = [&](std::string::size_type start) -> bool {
    start = content.find(':', start);
    if (start == std::string::npos) return false;
    ++start;
    while (start < content.size() && (content[start] == ' ' || content[start] == '\t')) ++start;
    return content.substr(start, 4) == "true";
  };

  size_t results_pos = content.find("\"results\"", pos);
  if (results_pos == std::string::npos) return true;

  size_t arr_start = content.find('[', results_pos);
  if (arr_start == std::string::npos) return true;

  size_t search_from = arr_start;
  while (true) {
    size_t obj_start = content.find('{', search_from);
    if (obj_start == std::string::npos || obj_start > content.find(']', arr_start)) break;

    RaceResult r;
    size_t p = obj_start;

    size_t rid_pos = content.find("\"race_id\"", p);
    if (rid_pos != std::string::npos) r.race_id = extract_string(rid_pos);

    size_t tid_pos = content.find("\"track_id\"", p);
    if (tid_pos != std::string::npos) r.track_id = extract_string(tid_pos);

    size_t tn_pos = content.find("\"track_name\"", p);
    if (tn_pos != std::string::npos) r.track_name = extract_string(tn_pos);

    size_t cid_pos = content.find("\"car_id\"", p);
    if (cid_pos != std::string::npos) r.car_id = static_cast<int>(extract_number(cid_pos));

    size_t dn_pos = content.find("\"driver_name\"", p);
    if (dn_pos != std::string::npos) r.driver_name = extract_string(dn_pos);

    size_t cl_pos = content.find("\"completed_laps\"", p);
    if (cl_pos != std::string::npos) r.completed_laps = static_cast<int>(extract_number(cl_pos));

    size_t tt_pos = content.find("\"total_time\"", p);
    if (tt_pos != std::string::npos) r.total_time = extract_number(tt_pos);

    size_t bl_pos = content.find("\"best_lap_time\"", p);
    if (bl_pos != std::string::npos) r.best_lap_time = extract_number(bl_pos);

    size_t sd_pos = content.find("\"session_date\"", p);
    if (sd_pos != std::string::npos) r.session_date = extract_string(sd_pos);

    size_t fi_pos = content.find("\"finished\"", p);
    if (fi_pos != std::string::npos) r.finished = extract_bool(fi_pos);

    size_t df_pos = content.find("\"dnf\"", p);
    if (df_pos != std::string::npos) r.dnf = extract_bool(df_pos);

    size_t dr_pos = content.find("\"dnf_reason\"", p);
    if (dr_pos != std::string::npos) r.dnf_reason = extract_string(dr_pos);

    size_t lt_pos = content.find("\"lap_times\"", p);
    if (lt_pos != std::string::npos) {
      size_t lap_arr_start = content.find('[', lt_pos);
      if (lap_arr_start != std::string::npos) {
        size_t lap_search = lap_arr_start;
        while (true) {
          size_t lap_obj_start = content.find('{', lap_search);
          if (lap_obj_start == std::string::npos || lap_obj_start > content.find(']', lap_arr_start)) break;

          LapTimeEntry lt;
          size_t lp = lap_obj_start;

          size_t ln_pos = content.find("\"lap_number\"", lp);
          if (ln_pos != std::string::npos) lt.lap_number = static_cast<int>(extract_number(ln_pos));

          size_t lt_pos2 = content.find("\"lap_time\"", lp);
          if (lt_pos2 != std::string::npos) lt.lap_time = extract_number(lt_pos2);

          size_t lv_pos = content.find("\"valid\"", lp);
          if (lv_pos != std::string::npos) lt.valid = extract_bool(lv_pos);

          size_t ts_pos = content.find("\"timestamp\"", lp);
          if (ts_pos != std::string::npos) lt.timestamp = extract_number(ts_pos);

          r.lap_times.push_back(lt);
          lap_search = lap_obj_start + 1;
        }
      }
    }

    db.add_result(r);
    search_from = obj_start + 1;
  }

  return true;
}

bool ResultStorage::save_race_result(const std::string& filepath, const RaceResult& result) {
  std::ofstream ofs(filepath);
  if (!ofs) return false;

  ofs << "{\n";
  ofs << "  \"race_id\": \"" << escape_json_string(result.race_id) << "\",\n";
  ofs << "  \"track_id\": \"" << escape_json_string(result.track_id) << "\",\n";
  ofs << "  \"track_name\": \"" << escape_json_string(result.track_name) << "\",\n";
  ofs << "  \"car_id\": " << result.car_id << ",\n";
  ofs << "  \"driver_name\": \"" << escape_json_string(result.driver_name) << "\",\n";
  ofs << "  \"completed_laps\": " << result.completed_laps << ",\n";
  ofs << "  \"total_time\": " << result.total_time << ",\n";
  ofs << "  \"best_lap_time\": " << result.best_lap_time << ",\n";
  ofs << "  \"session_date\": \"" << escape_json_string(result.session_date) << "\",\n";
  ofs << "  \"finished\": " << (result.finished ? "true" : "false") << ",\n";
  ofs << "  \"dnf\": " << (result.dnf ? "true" : "false") << ",\n";
  ofs << "  \"dnf_reason\": \"" << escape_json_string(result.dnf_reason) << "\",\n";
  ofs << "  \"lap_times\": [\n";
  for (size_t i = 0; i < result.lap_times.size(); ++i) {
    const auto& lt = result.lap_times[i];
    ofs << "    {\"lap_number\": " << lt.lap_number
        << ", \"lap_time\": " << lt.lap_time
        << ", \"valid\": " << (lt.valid ? "true" : "false")
        << ", \"timestamp\": " << lt.timestamp << "}";
    if (i + 1 < result.lap_times.size()) ofs << ",";
    ofs << "\n";
  }
  ofs << "  ]\n";
  ofs << "}\n";
  return true;
}

std::optional<RaceResult> ResultStorage::load_race_result(const std::string& filepath) {
  std::ifstream ifs(filepath);
  if (!ifs) return std::nullopt;

  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());

  RaceResult r;

  auto extract_string = [&](const std::string& key) -> std::string {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = content.find('"', pos + key.size() + 3);
    if (pos == std::string::npos) return "";
    ++pos;
    std::string out;
    while (pos < content.size()) {
      char c = content[pos++];
      if (c == '\\' && pos < content.size()) {
        char next = content[pos++];
        if (next == 'n') out += '\n';
        else if (next == 't') out += '\t';
        else if (next == '\\') out += '\\';
        else if (next == '"') out += '"';
        else { out += c; out += next; }
      } else if (c == '"') {
        break;
      } else {
        out += c;
      }
    }
    return out;
  };

  auto extract_number = [&](const std::string& key) -> double {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0;
    pos += key.size() + 3;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == ':')) ++pos;
    std::string num_str;
    while (pos < content.size() &&
           (std::isdigit(content[pos]) || content[pos] == '.' || content[pos] == '-' || content[pos] == 'e' || content[pos] == 'E')) {
      num_str += content[pos++];
    }
    if (num_str.empty()) return 0.0;
    return std::stod(num_str);
  };

  auto extract_bool = [&](const std::string& key) -> bool {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos += key.size() + 3;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == ':')) ++pos;
    return content.substr(pos, 4) == "true";
  };

  r.race_id = extract_string("race_id");
  r.track_id = extract_string("track_id");
  r.track_name = extract_string("track_name");
  r.car_id = static_cast<int>(extract_number("car_id"));
  r.driver_name = extract_string("driver_name");
  r.completed_laps = static_cast<int>(extract_number("completed_laps"));
  r.total_time = extract_number("total_time");
  r.best_lap_time = extract_number("best_lap_time");
  r.session_date = extract_string("session_date");
  r.finished = extract_bool("finished");
  r.dnf = extract_bool("dnf");
  r.dnf_reason = extract_string("dnf_reason");

  return r;
}

}
