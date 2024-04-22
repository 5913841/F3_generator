source /opt/intel/oneapi/setvars.sh
vtune -collect hotspots build/http_flood/server_main
vtune -collect uarch-exploration build/http_flood/server_main
