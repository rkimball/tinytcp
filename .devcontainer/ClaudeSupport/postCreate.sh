#!/usr/bin/env bash
# Link the container's Claude Code project history to the host's.
#
# Claude Code files session history under ~/.claude/projects/<workspace path
# with every non-alphanumeric character replaced by "-">. The container sees the
# workspace as /home/ubuntu/tinytcp, but history recorded on the host is filed under
# the host path, so it would not show up in the History tab. This links the
# container's project directory to the host's, wherever the repo lives on the host.
#
# Requires:
#   - the host's ~/.claude bind-mounted at ~/.claude (see devcontainer.json "mounts")
#   - HOST_WORKSPACE_FOLDER set to the host path (see devcontainer.json "containerEnv")
set -euo pipefail

encode() {
    printf '%s' "$1" | sed 's/[^A-Za-z0-9]/-/g'
}

if [ -z "${HOST_WORKSPACE_FOLDER:-}" ]; then
    echo "postCreate: HOST_WORKSPACE_FOLDER not set; skipping Claude history link" >&2
    exit 0
fi

projects_dir="$HOME/.claude/projects"
if [ ! -d "$HOME/.claude" ]; then
    echo "postCreate: $HOME/.claude not found; skipping Claude history link" >&2
    exit 0
fi
mkdir -p "$projects_dir"

host_name="$(encode "$HOST_WORKSPACE_FOLDER")"
container_name="$(encode "$(pwd)")"

if [ "$host_name" = "$container_name" ]; then
    exit 0
fi

host_dir="$projects_dir/$host_name"
container_dir="$projects_dir/$container_name"
mkdir -p "$host_dir"

# If the container already recorded history in a real directory, merge it into
# the host's directory (without overwriting) before replacing it with a link.
if [ -d "$container_dir" ] && [ ! -L "$container_dir" ]; then
    cp -an "$container_dir"/. "$host_dir"/
    rm -rf "$container_dir"
fi

# Relative link: both directories live in the same projects/ folder.
ln -sfn -- "$host_name" "$container_dir"
echo "postCreate: linked $container_dir -> $host_dir"
