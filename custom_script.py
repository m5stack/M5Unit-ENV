Import("env")

EXCLUDE_FILES = (env.GetProjectOption("custom_exclude_src_files") or []).split(' ')
print(EXCLUDE_FILES)

def skip_from_build(node):
  np = node.get_path()
  if any(ef in np for ef in EXCLUDE_FILES):
    return None
  return node

env.AddBuildMiddleware(skip_from_build, "*")
