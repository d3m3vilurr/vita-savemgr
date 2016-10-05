import sys
import json

try:
    print (json.load(sys.stdin)[0]['url'])
except:
    pass
