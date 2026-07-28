import { apiClient } from "./client";

export interface HealthEndpoint {
  method: string;
  path: string;
  description: string;
}

export interface HealthInfo {
  status: string;
  service: string;
  timestamp: string;
  uptime_seconds: number;
  endpoints: HealthEndpoint[];
}

export function getHealth() {
  return apiClient.get<HealthInfo>("/health");
}
