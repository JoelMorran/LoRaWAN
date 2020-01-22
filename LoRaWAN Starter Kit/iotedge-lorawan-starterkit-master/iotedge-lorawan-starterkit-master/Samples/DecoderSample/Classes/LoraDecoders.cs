// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SensorDecoderModule.Classes
{
    using System;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.IO;
    using System.Text;
    using System.Linq;
    using Newtonsoft.Json;

    using System.Data.SqlClient;

    internal static class LoraDecoders
    {
        private static string DecoderTest(string devEUI, byte[] payload, byte fport)
        {
            //Convert payload to string
            var result = Encoding.UTF8.GetString(payload);
            
            //Split payload
            string [] split = result.Split(new Char [] {','});

            try 
            { 
                //SQL connection parameters
                SqlConnectionStringBuilder builder = new SqlConnectionStringBuilder();
                builder.DataSource = "teamy-server.database.windows.net"; 
                builder.UserID = "teamy";            
                builder.Password = "Latrobe1234";     
                builder.InitialCatalog = "teamy_db";

                //SQL query setup
                StringBuilder sb = new StringBuilder();
                sb.Append("INSERT INTO TeamF (DeviceID, Timestamp, Axis1, Axis2, Axis3, Latitude, Longitude) ");
                sb.Append("VALUES ('");
                sb.Append(split[0]);
                sb.Append("', ");
                sb.Append("'");
                sb.Append(split[1]);
                sb.Append("', ");
                sb.Append(Convert.ToDouble(split[2]));
                sb.Append(", ");
                sb.Append(Convert.ToDouble(split[3]));
                sb.Append(", ");
                sb.Append(Convert.ToDouble(split[4]));
                sb.Append(", ");
                sb.Append(split[5]);
                sb.Append(", ");
                sb.Append(split[6]);
                sb.Append(")");
                String sql = sb.ToString();

                //Execute SQL query
                using (SqlConnection connection = new SqlConnection(builder.ConnectionString))
                {
                    SqlCommand command = new SqlCommand(sql, connection);
                    command.Connection.Open();
                    command.ExecuteNonQuery();

                    return JsonConvert.SerializeObject(new { value = "Payload successfully written to database" });
                }
            }
            catch (SqlException e)
            {
                return JsonConvert.SerializeObject(new { value = "Failed to write payload to database" });
            }
        }
    }
}