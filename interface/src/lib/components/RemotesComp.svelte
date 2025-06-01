<script lang="ts">
    import { onMount, onDestroy } from 'svelte';
    import SettingsCard from './SettingsCard.svelte';
    import GridForm from './GridForm.svelte';
    import { getJsonRest, postJsonRest } from '$lib/stores/rest';
    import { notifications } from "./toasts/notifications";
    import Button from './Button.svelte';
    import Text from './Text.svelte';
    import type { RemotesSettings, SpinRemote, ESPNowPeer } from '$lib/types/models';
    import Remote from '~icons/tabler/network';
    import Firmware from '~icons/tabler/refresh-alert';
    import Spinner from './Spinner.svelte';
	import Slider from './Slider.svelte';
	import RemoteComp from './RemoteComp.svelte';
	import Collapsible from './Collapsible.svelte';
	import Checkbox from './Checkbox.svelte';
    import { page } from '$app/stores';
    import { compareVersions } from 'compare-versions';
	import { get } from 'svelte/store';

    export let restPath: string;
    
    let remotes: SpinRemote[] = [];
    let remoteComps: RemoteComp[] = [];
    let remoteSettings: RemotesSettings = {
        isMaster: false,
    }
    let espnowPeers: ESPNowPeer[] = [];
    let newRemote = {
        hostname: '',
        ip: '',
        rxId: 99
    };
    let githubUpdate = {
        version: "",
        downloadLink: ""
    }
    let sequence = 0;

    $: hasUpdates = remotes.some(remote => remote.needsUpdate);

    async function getRemotesSettings() {
        return getJsonRest(restPath + '/settings', remoteSettings).then((data) => {
            remoteSettings = data;
        });
    }
    async function postRemotesSettings() {
        return postJsonRest(restPath + '/settings', remoteSettings).then((data) => {
            remoteSettings = data;
        });
    }

    async function getRemotes() {
        return getJsonRest(restPath, { remotes }).then((data) => {
            remotes = data.remotes;
        });
    }

    async function postRemotes() {
        return postJsonRest(restPath, { remotes }).then((data) => {
            return data;
        }).then(checkForUpdates).catch((error) => {
            notifications.error(`Failed to update remotes: ${error}`, 3000);
        });
    }

    async function addRemote() {
        if (newRemote.ip || newRemote.rxId) {
            return postJsonRest(restPath, { 
                addRemote: newRemote 
            }).then(() => {
                notifications.success(`Added remote ${newRemote.hostname}`, 3000);
                newRemote = { hostname: '', ip: '', rxId: 99 };
                return getRemotes();
            }).catch((error) => {
                notifications.error(`Failed to add remote: ${error}`, 3000);
            });
        }
    }

    async function removeRemote(index: number) {
        return postJsonRest(restPath, {
            removeRemote: {
                index: index
            }
        }).then(() => {
            notifications.success(`Removed remote ${remotes[index].hostname}`, 3000);
            return getRemotes();
        }).catch((error) => {
            notifications.error(`Failed to remove remote: ${error}`, 3000);
        });
    }

    async function checkForUpdates() {
        const githubUrl = `https://api.github.com/repos/${$page.data.github}/releases/latest`;
        try {
            const response = await fetch(githubUrl, {
                method: 'GET',
                headers: {
                    accept: 'application/vnd.github+json',
                    'X-GitHub-Api-Version': '2022-11-28'
                }
            });
            if (response.status !== 200) {
                throw new Error(`Failed to fetch latest release from ${githubUrl}`);
            }
            const results = await response.json();

            // iterate over assets and find the correct one
            for (let asset of results.assets) {
                // check if the asset is of type *.bin
                if (
                    asset.name.includes('.bin') &&
                    asset.name.includes($page.data.features.firmware_built_target) &&
                    !asset.name.includes('merged.bin')
                ) {
                    githubUpdate.version = results.tag_name;
                    githubUpdate.downloadLink = asset.browser_download_url;
                    break;
                }
            }

            checkRemotesForUpdates();
        } catch (error) {
            console.error('Error:', error);
        }
    }

    function checkRemotesForUpdates() {
        for (const remote of remotes) {
            if (remote.ip && remote.version && compareVersions(githubUpdate.version, remote.version) === 1) {
                if (!remote.needsUpdate) {
                    notifications.info(`Firmware update available for ${remote.hostname}.`, 5000);
                    remote.needsUpdate = true;
                    remotes = remotes;
                }
            }
        }
    }

    async function updateRemote(remote: SpinRemote) {
        let path = 'http://' + remote.ip + '/rest/downloadUpdate';
		try {
			const apiResponse = await fetch(path, {
				method: 'POST',
				body: JSON.stringify({ download_url: githubUpdate.downloadLink })
			});
            if (apiResponse.status == 200) remote.needsUpdate = false;
		} catch (error) {
			notifications.error("Error updating " + remote.hostname, 3000);
		}
	}

    async function updateAllRemotes() {
        for (const remote of remotes) {
            if (remote.needsUpdate) updateRemote(remote);
        }
    }

    async function launchSequenceAll() {
        for (const remote of remoteComps) {
            remote.runCommand(sequence, true);
        }
    }

    onMount(() => {
        getRemotesSettings();
        getRemotes().then(refreshAllRemotes);
        getESPNowPeers();
    });

    async function refreshAllRemotes() {
        return Promise.all(
            remoteComps.map(remote => remote.getDiag())
        ).then(() => {
            checkForUpdates();
        });
    }

    async function getESPNowPeers() {
        return getJsonRest('rest/espnow/peers', espnowPeers).then((data) => {
            espnowPeers = data;
            console.log(data)
        });
    }
</script>

<SettingsCard>
    <Remote slot="icon" class="flex-shrink-0 mr-2 h-6 w-6 self-end" />
    <span slot="title">Remotes Manager</span>

        <Slider
            label="Select Command"
            min={0}
            max={100}
            step={1}
            disabled={remotes.length == 0}
            hasNumber
            bind:value={sequence}
        />
        <Button
            label="Trigger All"
            onClick={launchSequenceAll}
        />

    <!-- Remotes list -->
    <Collapsible open={remotes.length > 0}>
        <span slot="title">Registered Remotes ({remotes.length})</span>
        {#if remotes.length === 0}
            <p class="text-gray-500 dark:text-gray-400">No remotes configured</p>
        {:else}
            <div class="grid gap-4">
                {#each remotes as remote, index}
                    <div class="bg-base-200 p-0 rounded-lg flex items-center justify-between gap-2">
                                <!-- Open Link in new tab if IP is set -->
                        {#if remote.ip}
                            <a href={`http://${remote.ip}`} target="_blank" rel="noopener noreferrer">
                            <div>
                                <p class="font-semibold">{remote.hostname}</p>
                                <p class="text-sm text-gray-600 dark:text-gray-400">
                                    IP: {remote.ip || 'Not Connected'} | RX ID: {remote.rxId} | FW: {remote.version}
                                </p>
                            </div>
                            </a>
                        {:else}
                            <div>
                                <p class="font-semibold">{remote.hostname}</p>
                                <p class="text-sm text-gray-600 dark:text-gray-400">
                                    IP: {remote.ip || 'Not Connected'} | RX ID: {remote.rxId}
                                </p>
                            </div>
                        {/if}
                        <div class="flex-grow"></div>
                        {#if remote.needsUpdate}
                            <button
                                class="btn btn-square btn-ghost h-7 w-7 ml-2"
                                on:click={() => updateRemote(remote)}
                            >
                                <div class="h-7 content-center items-center self-start">
                                    <span class="indicator-item indicator-top indicator-center badge badge-info badge-xs top-2 scale-75 lg:top-1">
                                        {githubUpdate.version}
                                    </span>
                                    <Firmware class="h-7 w-7" />
                                </div>
                            </button>
                        {/if}
                        <Button
                            label="Refresh"
                            onClick={() => {
                                remoteComps[index].getDiag().then(checkForUpdates);
                            }}
                        />
                        <Button
                            onClick={() => removeRemote(index)}
                            label="Remove"
                        />
                    </div>
                {/each}
            </div>
             
            {#if hasUpdates}
                <Button
                    label="Update All"
                    onClick={updateAllRemotes}
                />
            {/if}
        {/if}
    </Collapsible>

    <!-- Add new remote form -->
    <Collapsible>
        <span slot="title">Import Remotes</span>
        <GridForm>
            <Checkbox
                label="Is Master"
                bind:value={remoteSettings.isMaster}
                onChange={postRemotesSettings}
            />
            <Text
                label="Hostname"
                bind:value={newRemote.hostname}
            />
            <Text
                label="IP Address"
                bind:value={newRemote.ip}
            />
            <Slider
                label="RX ID"
                min={1}
                max={127}
                step={1}
                bind:value={newRemote.rxId}
            />
        </GridForm>

        <div class="flex flex-row flex-wrap justify-between gap-x-2">
            <Button
                onClick={() => postJsonRest(restPath, { scanMDNS: true }).then(() => {
                    setTimeout(() => {
                        getRemotes();
                    }, 2000);
                })}
                label="Scan"
            />
            <Button
                onClick={addRemote}
                label="Add New"
            />
            <div class="flex-grow"></div>
            <Button
                onClick={() => {
                    remotes = [];
                    postJsonRest(restPath, { clearRemotes: true });
                }}
                label="Clear All"
            />
        </div>
    </Collapsible>
</SettingsCard>

{#each remotes as remote, index}
    <RemoteComp
        bind:remote={remote}
        bind:espnowPeers={espnowPeers}
        bind:this={remoteComps[index]}
        onChange={postRemotes}
    />
{/each}

<style>
    .grid {
        display: grid;
        gap: 1rem;
    }
</style>