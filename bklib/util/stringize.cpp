#include "pch.hpp"
#include "stringize.hpp"

std::ostream&
bklib::util::flatten_stringized(std::ostream& out, stringized_t const& s) {
    static unsigned const INDENT = 2;
    //! (pointer, depth)
    typedef std::pair<stringized_base*, size_t> pair_t;

    std::queue<pair_t> items;
    items.push(std::make_pair(s.get(), 0));

    while (!items.empty()) {
        auto const ptr   = items.front().first;
        auto const depth = items.front().second;

        out << std::string(depth * INDENT, ' ')
            << "[" <<  ptr->type << "] "
            << ptr->name << " = " << ptr->value
            << std::endl;

        if (ptr->get_children()) {
            for (auto& i : *ptr->get_children()) {
                items.push(std::make_pair(i.get(), depth + 1));
            }
        }

        items.pop();
    }

    return out;
}
