class UnknownBuildType(Exception):
    def __init__(self, buildtype_name):
        print("Unknown BuildType : {}".format(buildtype_name))


class MissingEnvironmentVariable(Exception):
    def __init__(self, env_var_name):
        print("Missing environment variable : {}".format(env_var_name))
