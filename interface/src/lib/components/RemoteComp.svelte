<script lang="ts">
    import { getJsonRest, postJsonRest } from '$lib/stores/rest';
    import { getJsonRestWithCanFallback, postJsonRestWithCanFallback, getJsonRestWithHostnameFallback } from '$lib/stores/remote';
    import Spinner from './Spinner.svelte';
    import type { ESPNowPeer, SequencerStatus, SpinDiagnostics, SpinRemote } from '$lib/types/models';
	import SettingsCard from './SettingsCard.svelte';
	import StatusPanel from './StatusPanel.svelte';
	import Collapsible from './Collapsible.svelte';
    import Remote from '~icons/tabler/network';
    import Firmware from '~icons/tabler/refresh-alert';
	import { onMount, onDestroy } from 'svelte';
	import Slider from './Slider.svelte';
    import Button from './Button.svelte';
    import { openModal, closeAllModals } from 'svelte-modals';
    import ConfirmDialog from './ConfirmDialog.svelte';
    import { notifications } from '$lib/components/toasts/notifications';
    import Cancel from '~icons/tabler/x';
    import CloudDown from '~icons/tabler/cloud-download';
    import GithubUpdateDialog from './GithubUpdateDialog.svelte';
    import { compareVersions } from 'compare-versions';
	import Checkbox from './Checkbox.svelte';
	import { sequence } from '@sveltejs/kit/hooks';
	import SequencerProgressBar from './SequencerProgressBar.svelte';
	import GridForm from './GridForm.svelte';

    export let remote: SpinRemote;
    export let diag: SpinDiagnostics = {} as SpinDiagnostics;
    export let onChange: () => void;
    export let espnowPeers: ESPNowPeer[] = [];

    let sequencerStatus: SequencerStatus;
    let httpLatency: number = 0;
    let lastHttpRequestTime: number = 0;

    export async function getDiag() {
        lastHttpRequestTime = Date.now();
        return getJsonRestWithHostnameFallback("/spin-seq/diag", diag, remote.ip, remote.hostname).then((data) => {
            diag = data;
            httpLatency = Date.now() - lastHttpRequestTime;
            updateRemote();
            return diag;
        }).catch((error) => {
            console.error("Failed to get diagnostics: ", error);
        });
    }

    async function getSequencerData() {
        if (remote.ip) {
            let path = "http://" + remote.ip + "/rest/spin-seq/diag/sequencer";
            return getJsonRest(path, diag.sequencer, {signal: AbortSignal.timeout(1000)}).then((data) => {
                diag.sequencer = data;
                return diag;
            }).catch((error) => {
                console.error("Failed to get sequencer data: ", error);
            });
        }
    }

    async function getSequencerStatus() {
        if (remote.ip) {
            lastHttpRequestTime = Date.now();
            let path = "http://" + remote.ip + "/rest/spin-seq/sequencer/status";
            return getJsonRest(path, sequencerStatus, {signal: AbortSignal.timeout(1000)}).then((data) => {
                sequencerStatus = data;
                httpLatency = Date.now() - lastHttpRequestTime;
                return sequencerStatus;
            }).catch((error) => {
                console.error("Failed to get sequencer status: ", error);
            });
        }
    }

    export async function runCommand(command: number, enableSeq = false) {
        let path = "http://" + (remote.ip || remote.hostname) + "/rest/spin-seq/sequencer";
        return postJsonRest(path, {control:{execute:command}})
            .then(() => {if (enableSeq && !diag?.sequencer?.isRunning) {setSequencerState(true); diag.sequencer.isRunning = true;}});
    }

    export async function setSequencerState(state: boolean) {
        let path = "http://" + (remote.ip || remote.hostname) + "/rest/spin-seq/sequencer";
        return postJsonRest(path, {control:{run:state}});
    }

    async function updateRemote() {
        let changed = false;
        if (diag.wifi && diag.wifi.hostname != remote.hostname) {
            remote.hostname = diag.wifi?.hostname;
            changed = true;
        }
        if (diag.wifi && diag.wifi?.ip != remote.ip) {
            remote.ip = diag.wifi?.ip;
            changed = true;
        }
        if (diag.can && diag.can?.rxId != remote.rxId) {
            remote.rxId = diag.can?.rxId;
            changed = true;
        }
        if (diag.mcu && diag.mcu?.version != remote.version) {
            remote.version = diag.mcu.version;
            changed = true;
        }
        if (diag.espnow && diag.espnow?.macAddress != remote.macAddress) {
            remote.macAddress = diag.espnow.macAddress;
            changed = true;
        }
        if (changed) {
            onChange();
        }
    }

    let intervalID: any;
    onMount(() => {
        intervalID = setInterval(() => {
            if (diag?.sequencer?.isRunning) getSequencerStatus();
        }, 1278);
        getSequencerStatus();
        if (!remote.hostname || !remote.ip || !remote.version) {
            getDiag();
        }
    });
    onDestroy(() => {
        clearInterval(intervalID);
    });

</script>

<SettingsCard>
    <Remote slot="icon" class="lex-shrink-0 mr-2 h-6 w-6 self-end" />
    <span slot="title" class="h-7 flex items-center gap-2 w-full">
        <a href={"http://" + (remote.ip ? remote.ip : (remote.hostname + ".local"))} target="_blank" rel="noopener noreferrer">
            {remote.hostname || remote.ip}
        </a>
        <div class="flex-grow"></div>
        {#if espnowPeers?.length > 0 && remote.macAddress}
            {#each espnowPeers.filter(p => p && p.mac === remote.macAddress) as peer}
                {@const lossRatio = (peer.numLost / peer.numSent * 100).toFixed(1)}
                <span class="text-sm text-gray-500">
                    (ESPNow: {peer.numReceived}/{peer.numSent} msgs, {peer.pingMeanTime.toFixed(1)}ms, {peer.numLost} lost ({lossRatio}%){httpLatency ? ", HTTP: " + httpLatency + "ms" : ""})
                </span>
            {/each}
        {/if}
    </span>
    {#if sequencerStatus}
        <GridForm>
            <Checkbox
                label="Sequencer State"
                bind:value={sequencerStatus.isRunning}
                onChange={() => setSequencerState(sequencerStatus.isRunning)}
            />
            <SequencerProgressBar sequencerStatus={sequencerStatus} />
            <!-- <Slider
                label="Sequencer Command"
                min={0}
                max={diag.sequencer.numCommands - 1}
                step={1}
                disabled={diag.sequencer.numCommands == 0}
                hasNumber
                onChange={() => runCommand(diag.sequencer.selectedCommand)}
                bind:value={diag.sequencer.selectedCommand}
            /> -->
        </GridForm>
    {/if}
    <Collapsible>
        <span slot="title">Diagnostics</span>
        {#await getDiag()}
            <Spinner></Spinner>
        {:then diag}
            <StatusPanel
                diag={diag}
            />
        {/await}
        {#if remote.ip}
        <Button
            label="Open UI"
            onClick={() => {
                window.open("http://" + remote.ip, "_blank");
            }}
        />
        {/if}
    </Collapsible>
</SettingsCard>