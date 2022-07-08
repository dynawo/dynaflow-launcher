class UnknownBuildType(Exception):
    def __init__(self, buildtype_name):
        print("Unknown BuildType : {}".format(buildtype_name))
