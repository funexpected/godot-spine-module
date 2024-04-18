from spine_versions import spine_versions, default_versions

def can_build(env, platform):
  return True
  
  
def configure(env):
  from SCons.Script import BoolVariable, Variables, Help
  envvars = Variables()
  for version in spine_versions.keys():
    underscored_version = version.replace(".", "_")
    envvars.Add(BoolVariable(
      "spine_runtime_%s" % underscored_version, 
      "Compile with %s runtime supported" % version, 
      version in default_versions
    ))
    envvars.Update(env)
    Help(envvars.GenerateHelpText(env))

  #env.Append(CPPFLAGS=['-DNEED_LONG_INT'])
  pass
  
  
  
