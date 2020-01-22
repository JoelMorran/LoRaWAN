import pandas as pd
import urllib
from sqlalchemy import create_engine

engine = create_engine('mssql+pyodbc:///?odbc_connect=' +
    urllib.quote_plus('DRIVER=FreeTDS;SERVER=teamy-server.database.windows.net;PORT=1433;DATABASE=teamy_db;UID=teamy;PWD=Latrobe1234;TDS_Version=8.0;')
)

df = pd.read_sql('SELECT TOP 200 * FROM TeamF', engine)

print(df.iloc[0]['X'])
print(df.iloc[0]['Y'])
print(df.iloc[0]['Z'])