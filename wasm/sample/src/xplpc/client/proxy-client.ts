export interface XProxyClient {
    call<T>(data: string): T;
    callAsync<T>(data: string): Promise<T>;
}
