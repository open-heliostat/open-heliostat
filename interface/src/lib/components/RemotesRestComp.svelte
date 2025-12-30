<script lang="ts">
    import { onMount } from 'svelte';
    import SettingsCard from './SettingsCard.svelte';
    import Text from './Text.svelte';
    import Checkbox from './Checkbox.svelte';
    import Button from './Button.svelte';
    import Spinner from './Spinner.svelte';
    import { getJsonRest, postJsonRest } from '$lib/stores/rest';
    import { notifications } from '$lib/components/toasts/notifications';
    import Radar from '~icons/tabler/radar';
    import type { Remote } from '$lib/types/models';

    let remotes: Remote[] = [];
    let settings = { isMaster: false };
    let newRemote = { hostname: '', ip: '', macAddress: '', rxId: '' };
    let loading = false;
    let scanning = false;

    async function refresh() {
        loading = true;
        await getJsonRest('/rest/remote', { remotes: [], settings }).then((data) => {
            remotes = data.remotes ?? [];
            settings = data.settings ?? settings;
        });
        loading = false;
    }

    async function saveSettings() {
        await postJsonRest('/rest/remote/settings', settings);
    }

    async function scanMdns() {
        scanning = true;
        await postJsonRest('/rest/remote/scanMDNS', {});
        await refresh();
        scanning = false;
    }

    async function clearRemotes() {
        await postJsonRest('/rest/remote/clearRemotes', {});
        await refresh();
    }

    async function addRemote() {
        const payload: Record<string, any> = {};
        if (newRemote.hostname) payload.hostname = newRemote.hostname;
        if (newRemote.ip) payload.ip = newRemote.ip;
        if (newRemote.macAddress) payload.macAddress = newRemote.macAddress;
        if (newRemote.rxId) {
            const rxIdNum = Number(newRemote.rxId);
            if (Number.isNaN(rxIdNum)) {
                notifications.error('RX ID must be numeric', 4000);
                return;
            }
            payload.rxId = rxIdNum;
        }
        if (!payload.ip && payload.rxId === undefined) {
            notifications.error('Provide an IP or RX ID to add a remote', 4000);
            return;
        }
        await postJsonRest('/rest/remote/addRemote', payload);
        newRemote = { hostname: '', ip: '', macAddress: '', rxId: '' };
        await refresh();
    }

    async function removeRemote(index: number) {
        await postJsonRest('/rest/remote/removeRemote', { index });
        await refresh();
    }

    onMount(refresh);
</script>

<SettingsCard>
    {#snippet icon()}
        <Radar class="flex-shrink-0 mr-2 h-6 w-6 self-end" />
    {/snippet}
    {#snippet title()}
        <span>Remotes</span>
    {/snippet}
    {#if loading}
        <div class="p-4"><Spinner /></div>
    {:else}
        <div class="flex flex-wrap gap-4 items-center">
            <Checkbox label="Master" bind:value={settings.isMaster} onChange={saveSettings} />
            <Button onClick={scanMdns} label={scanning ? 'Scanning...' : 'Scan mDNS'} />
            <Button onClick={clearRemotes} label="Clear" />
        </div>

        <div class="mt-4">
            {#if remotes.length > 0}
                <div class="overflow-x-auto">
                    <table class="table table-zebra w-full">
                        <thead>
                            <tr>
                                <th>Hostname</th>
                                <th>IP</th>
                                <th>MAC</th>
                                <th class="w-20">RX ID</th>
                                <th></th>
                            </tr>
                        </thead>
                        <tbody>
                            {#each remotes as remote, index}
                                <tr>
                                    <td>{remote.hostname}</td>
                                    <td>{remote.ip}</td>
                                    <td>{remote.macAddress}</td>
                                    <td>{remote.rxId}</td>
                                    <td class="text-right">
                                        <button class="btn btn-sm btn-error" on:click={() => removeRemote(index)}>
                                            Remove
                                        </button>
                                    </td>
                                </tr>
                            {/each}
                        </tbody>
                    </table>
                </div>
            {:else}
                <p class="text-sm text-base-content/70">No remotes saved yet.</p>
            {/if}
        </div>

        <div class="mt-6 grid gap-2 sm:grid-cols-2">
            <Text label="Hostname" bind:value={newRemote.hostname} />
            <Text label="IP" bind:value={newRemote.ip} />
            <Text label="MAC Address" bind:value={newRemote.macAddress} />
            <div>
                <label class="label cursor-pointer" for="rx-id-input">
                    <span class="mr-4">RX ID</span>
                </label>
                <input
                    id="rx-id-input"
                    type="number"
                    class="input"
                    bind:value={newRemote.rxId}
                />
            </div>
            <div class="sm:col-span-2">
                <Button onClick={addRemote} label="Add Remote" />
            </div>
        </div>
    {/if}
</SettingsCard>
