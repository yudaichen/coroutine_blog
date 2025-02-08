module;

/*
#include <boost/json/object.hpp>
#include <boost/json/array.hpp>
#include <boost/mysql/datetime.hpp>
*/
import boost;
import std;
export module blog_column_entity;

export namespace fast::entity
{
struct BlogColumn
{
  public:
    std::uint64_t column_id;
    std::string column_name;
    std::string column_description;
    std::uint64_t pid;
    std::vector<BlogColumn> children;
};

boost::json::value column_to_json(const BlogColumn &column)
{
    boost::json::object j;
    j["column_id"] = column.column_id;
    j["column_name"] = column.column_name;
    j["column_description"] = column.column_description;
    j["pid"] = column.pid;

    if (!column.children.empty())
    {
        boost::json::array children_array;
        for (const auto &child : column.children)
        {
            children_array.push_back(column_to_json(child));
        }
        j["children"] = children_array;
    }
    return j;
}

boost::json::value columns_to_json(const std::vector<BlogColumn> &columns)
{
    boost::json::array j_columns;
    for (const auto &column : columns)
    {
        j_columns.push_back(column_to_json(column));
    }
    return j_columns;
}

void build_column_tree(std::vector<fast::entity::BlogColumn> &columns)
{
    std::map<std::uint64_t, std::vector<fast::entity::BlogColumn *>> children_map;

    for (auto &column : columns)
    {
        children_map[column.pid].push_back(&column);
    }

    for (auto &column : columns)
    {
        if (children_map.contains(column.column_id))
        {
            for (auto *child : children_map[column.column_id])
            {
                column.children.push_back(std::move(*child));
            }
        }
    }
    std::erase_if(columns, [](const fast::entity::BlogColumn &c) { return c.pid != 0; });
}

void build_column_tree_recursive(std::vector<fast::entity::BlogColumn> &all_columns,
                                 std::vector<fast::entity::BlogColumn> &current_level, std::uint64_t parent_id)
{
    for (auto &column : all_columns)
    {
        if (column.pid == parent_id)
        {
            current_level.push_back(column);
        }
    }

    std::sort(current_level.begin(), current_level.end(),
              [](const BlogColumn &a, const BlogColumn &b) { return a.column_id < b.column_id; });

    for (auto &column : current_level)
    {
        std::vector<BlogColumn> children;
        build_column_tree_recursive(all_columns, children, column.column_id);
        column.children = std::move(children);
    }
}

} // namespace fast::entity
