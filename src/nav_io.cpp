#include "gmenu/menu.hpp"

#include <fstream>
#include <gsexp/sexp.hpp>
#include <optional>
#include <sstream>

namespace gmenu {

namespace {

void set_link_for_direction(NavLinks& links, NavDirection direction, WidgetId target) {
    switch (direction) {
    case NavDirection::Up:
        links.up = target;
        break;
    case NavDirection::Down:
        links.down = target;
        break;
    case NavDirection::Left:
        links.left = target;
        break;
    case NavDirection::Right:
        links.right = target;
        break;
    }
}

void write_link(std::ostream& out, const char* name, WidgetId target) {
    if (target != invalid_widget) {
        out << "    (" << name << " " << target << ")\n";
    }
}

bool read_file(const std::filesystem::path& path, std::string& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

bool read_direction(gsexp::FormView form, const char* name, NavLinks& links,
                    NavDirection direction) {
    std::optional<int> value = form.get_int(name);
    if (!value || *value <= 0) {
        return false;
    }
    set_link_for_direction(links, direction, static_cast<WidgetId>(*value));
    return true;
}

bool parse_resolution(gsexp::FormView form, NavScope& scope) {
    gsexp::Node resolution = form.find("resolution");
    if (!resolution.valid()) {
        return true;
    }
    gsexp::FormView resolution_form(resolution);
    std::optional<int> width = resolution_form.get_int("width");
    std::optional<int> height = resolution_form.get_int("height");
    if (!width || !height || *width <= 0 || *height <= 0) {
        return false;
    }
    scope.width = *width;
    scope.height = *height;
    scope.match_resolution = true;
    return true;
}

bool parse_form_factor(gsexp::FormView form, NavScope& scope) {
    std::optional<std::string_view> text = form.get_string_view("form_factor");
    if (!text) {
        return true;
    }
    scope.form_factor = glayout::form_factor_from_string(*text);
    scope.match_form_factor = true;
    return true;
}

bool parse_link(gsexp::Node node, NavGraphLink& out) {
    gsexp::FormView form(node);
    std::optional<int> screen = form.get_int("screen");
    std::optional<int> widget = form.get_int("widget");
    if (!screen || !widget || *screen <= 0 || *widget <= 0) {
        return false;
    }

    out.scope.screen = static_cast<ScreenId>(*screen);
    out.widget = static_cast<WidgetId>(*widget);
    if (std::optional<int> layout = form.get_int("layout")) {
        out.scope.layout_id = *layout;
    }
    if (!parse_resolution(form, out.scope) || !parse_form_factor(form, out.scope)) {
        return false;
    }

    read_direction(form, "up", out.links, NavDirection::Up);
    read_direction(form, "down", out.links, NavDirection::Down);
    read_direction(form, "left", out.links, NavDirection::Left);
    read_direction(form, "right", out.links, NavDirection::Right);
    return true;
}

} // namespace

bool Menu::load_nav_file(const std::filesystem::path& path) {
    std::string text;
    if (!read_file(path, text)) {
        return false;
    }

    gsexp::ParseResult parsed = gsexp::parse_owned(std::move(text));
    if (!parsed.ok || parsed.root_count() == 0) {
        return false;
    }

    gsexp::Node root = parsed.root(0);
    if (!root.is_list() || !root.head().is_atom("gmenu_nav")) {
        return false;
    }

    std::vector<NavGraphLink> records;
    for (gsexp::Node child : root.children()) {
        if (!child.is_list() || !child.head().is_atom("link")) {
            continue;
        }
        NavGraphLink record;
        if (parse_link(child, record)) {
            records.push_back(record);
        }
    }

    nav_graph_records = std::move(records);
    nav_dirty_flag = false;
    return true;
}

bool Menu::save_nav_file(const std::filesystem::path& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << "(gmenu_nav\n";
    for (const NavGraphLink& record : nav_graph_records) {
        file << "  (link\n";
        file << "    (screen " << record.scope.screen << ")\n";
        if (record.scope.layout_id != 0) {
            file << "    (layout " << record.scope.layout_id << ")\n";
        }
        if (record.scope.match_resolution) {
            file << "    (resolution (width " << record.scope.width << ") (height "
                 << record.scope.height << "))\n";
        }
        if (record.scope.match_form_factor) {
            file << "    (form_factor " << glayout::to_string(record.scope.form_factor) << ")\n";
        }
        file << "    (widget " << record.widget << ")\n";
        write_link(file, "up", record.links.up);
        write_link(file, "down", record.links.down);
        write_link(file, "left", record.links.left);
        write_link(file, "right", record.links.right);
        file << "  )\n";
    }
    file << ")\n";
    return file.good();
}

} // namespace gmenu
